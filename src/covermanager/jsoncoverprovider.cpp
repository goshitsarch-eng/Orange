#include "covermanager/jsoncoverprovider.h"

#include "covermanager/albumcoverfetchersearch.h"
#include "utilities/jsonutils.h"

#include <cstring>
#include <glib.h>

JsonCoverProvider::JsonCoverProvider(std::string name, std::string url_template)
    : name_(std::move(name)), url_template_(std::move(url_template)) {}

void JsonCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  Search(song, network, [callback](const CoverProviderSearchResults &results) {
    if (results.empty()) {
      callback({}, "No cover URL in provider response");
      return;
    }
    if (!results.front().image_data.empty()) {
      callback(results.front().image_data, {});
      return;
    }
    callback(results.front().image_url, {});
  });
}

void JsonCoverProvider::Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) {
  if (!network || song.album().empty()) {
    callback({});
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
  const std::string artist = song.EffectiveAlbumartist();
  const std::string album = song.album();
  network->Get(url, [this, callback, artist, album](const NetworkAccessManager::Response &response) {
    CoverProviderSearchResults results;
    if (!response.ok()) {
      callback(results);
      return;
    }
    if (JsonUtils::LooksLikeImage(response.body)) {
      results.push_back(AlbumCoverFetcherSearch::FromHit(name(), artist, album, {}, 0, 0, response.body));
      callback(results);
      return;
    }
    for (const std::string &image_url : JsonUtils::FindAllCoverUrls(response.body)) {
      results.push_back(AlbumCoverFetcherSearch::FromHit(name(), artist, album, image_url));
    }
    callback(results);
  });
}
