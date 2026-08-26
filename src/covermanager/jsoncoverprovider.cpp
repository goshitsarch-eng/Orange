#include "covermanager/jsoncoverprovider.h"

#include "utilities/jsonutils.h"

#include <cstring>
#include <glib.h>

JsonCoverProvider::JsonCoverProvider(std::string name, std::string url_template)
    : name_(std::move(name)), url_template_(std::move(url_template)) {}

void JsonCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || song.album().empty()) {
    callback({}, "No album");
    return;
  }
  std::string url = url_template_;
  auto replace = [&url](const std::string &token, const std::string &value) {
    gchar *escaped = g_uri_escape_string(value.c_str(), nullptr, TRUE);
    size_t pos = 0;
    while ((pos = url.find(token, pos)) != std::string::npos) {
      url.replace(pos, token.size(), escaped ? escaped : value);
      pos += escaped ? strlen(escaped) : value.size();
    }
    g_free(escaped);
  };
  replace("{artist}", song.EffectiveAlbumartist());
  replace("{album}", song.album());
  replace("{title}", song.title());
  network->Get(url, [callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({}, response.error.empty() ? "Cover request failed" : response.error);
      return;
    }
    if (JsonUtils::LooksLikeImage(response.body)) {
      callback(response.body, {});
      return;
    }
    const std::string image_url = JsonUtils::FindCoverUrl(response.body);
    if (image_url.empty()) {
      callback({}, "No cover URL in provider response");
      return;
    }
    callback(image_url, {});
  });
}
