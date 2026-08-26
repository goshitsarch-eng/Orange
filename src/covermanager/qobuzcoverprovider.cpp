#include "covermanager/qobuzcoverprovider.h"

#include "core/settings.h"
#include "qobuz/qobuzservice.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

const int QobuzCoverProvider::kLimit = 10;

namespace {

std::string ObjectString(JsonObject *object, const char *name) {
  if (!object || !json_object_has_member(object, name) || !JSON_NODE_HOLDS_VALUE(json_object_get_member(object, name))) {
    return {};
  }
  if (json_node_get_value_type(json_object_get_member(object, name)) != G_TYPE_STRING) {
    return {};
  }
  const char *value = json_object_get_string_member(object, name);
  return value ? value : "";
}

}  // namespace

std::string QobuzCoverProvider::SearchUrl(const std::string &artist, const std::string &album, const std::string &title) {
  std::string resource;
  std::string query = artist;
  if (album.empty() && !title.empty()) {
    resource = "track/search";
    if (!query.empty()) {
      query += " ";
    }
    query += title;
  } else {
    resource = "album/search";
    if (!album.empty()) {
      if (!query.empty()) {
        query += " ";
      }
      query += album;
    }
  }
  return std::string(QobuzService::kApiUrl) + "/" + resource + "?query=" + StrUtils::UriEscape(query) + "&limit=" + std::to_string(kLimit);
}

std::vector<QobuzCoverProvider::SearchResult> QobuzCoverProvider::ParseResults(const std::string &json) {
  std::vector<SearchResult> results;
  if (json.empty()) {
    return results;
  }
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return results;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return results;
  }
  JsonObject *object = json_node_get_object(root);
  const char *group_key = json_object_has_member(object, "albums") ? "albums" : (json_object_has_member(object, "tracks") ? "tracks" : nullptr);
  if (!group_key || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, group_key))) {
    g_object_unref(parser);
    return results;
  }
  JsonObject *group = json_object_get_object_member(object, group_key);
  if (!json_object_has_member(group, "items") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(group, "items"))) {
    g_object_unref(parser);
    return results;
  }
  JsonArray *items = json_object_get_array_member(group, "items");
  const guint n = json_array_get_length(items);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item_node = json_array_get_element(items, i);
    if (!item_node || !JSON_NODE_HOLDS_OBJECT(item_node)) {
      continue;
    }
    JsonObject *item = json_node_get_object(item_node);
    JsonObject *album = item;
    if (json_object_has_member(item, "album") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(item, "album"))) {
      album = json_object_get_object_member(item, "album");
    }
    if (!json_object_has_member(album, "artist") || !json_object_has_member(album, "image") ||
        !JSON_NODE_HOLDS_OBJECT(json_object_get_member(album, "artist")) || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(album, "image"))) {
      continue;
    }
    const std::string artist = ObjectString(json_object_get_object_member(album, "artist"), "name");
    const std::string title = ObjectString(album, "title");
    const std::string cover = ObjectString(json_object_get_object_member(album, "image"), "large");
    if (title.empty() || cover.empty()) {
      continue;
    }
    SearchResult result;
    result.artist = artist;
    result.album = Song::AlbumRemoveDiscMisc(title);
    result.image_url = cover;
    results.push_back(result);
  }
  g_object_unref(parser);
  return results;
}

void QobuzCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || (song.EffectiveAlbumartist().empty() && song.album().empty() && song.title().empty())) {
    callback({}, "No artist, album, or title");
    return;
  }
  Settings settings;
  settings.BeginGroup("Qobuz");
  const std::string app_id = settings.Value("appid");
  const std::string token = settings.Value("token").empty() ? settings.Value("user_auth_token") : settings.Value("token");
  if (app_id.empty() || token.empty()) {
    callback({}, "Qobuz is not signed in");
    return;
  }
  network->Get(SearchUrl(song.EffectiveAlbumartist(), song.album(), song.title()),
               [callback](const NetworkAccessManager::Response &response) {
                 if (!response.ok()) {
                   callback({}, response.error.empty() ? "Qobuz cover request failed" : response.error);
                   return;
                 }
                 const std::vector<SearchResult> results = ParseResults(response.body);
                 if (results.empty()) {
                   callback({}, "No Qobuz cover");
                   return;
                 }
                 callback(results.front().image_url, {});
               },
               {{"X-App-Id", app_id}, {"X-User-Auth-Token", token}});
}
