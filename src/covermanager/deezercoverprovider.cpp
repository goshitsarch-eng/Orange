#include "covermanager/deezercoverprovider.h"

#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

const char *DeezerCoverProvider::kApiUrl = "https://api.deezer.com";
const int DeezerCoverProvider::kLimit = 10;

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

std::string DeezerCoverProvider::SearchUrl(const std::string &artist, const std::string &album, const std::string &title) {
  std::string resource;
  std::string query = artist;
  if (album.empty() && !title.empty()) {
    resource = "search/track";
    if (!query.empty()) {
      query += " ";
    }
    query += title;
  } else {
    resource = "search/album";
    if (!album.empty()) {
      if (!query.empty()) {
        query += " ";
      }
      query += album;
    }
  }
  return std::string(kApiUrl) + "/" + resource + "?output=json&q=" + StrUtils::UriEscape(query) + "&limit=" + std::to_string(kLimit);
}

std::vector<DeezerCoverProvider::SearchResult> DeezerCoverProvider::ParseResults(const std::string &json) {
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
  const char *data_key = json_object_has_member(object, "data") ? "data" : (json_object_has_member(object, "DATA") ? "DATA" : nullptr);
  if (!data_key || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, data_key))) {
    g_object_unref(parser);
    return results;
  }
  JsonArray *array = json_object_get_array_member(object, data_key);
  const guint n = json_array_get_length(array);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item = json_array_get_element(array, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    JsonObject *entry = json_node_get_object(item);
    JsonObject *album = entry;
    if (json_object_has_member(entry, "album") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(entry, "album"))) {
      album = json_object_get_object_member(entry, "album");
    }
    if (ObjectString(album, "type") != "album") {
      continue;
    }
    if (!json_object_has_member(entry, "artist") || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(entry, "artist"))) {
      continue;
    }
    JsonObject *artist_obj = json_object_get_object_member(entry, "artist");
    const std::string artist = ObjectString(artist_obj, "name");
    const std::string title = ObjectString(album, "title");
    std::string cover = ObjectString(album, "cover_xl");
    if (cover.empty()) {
      cover = ObjectString(album, "cover_big");
    }
    if (cover.empty()) {
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

void DeezerCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || (song.EffectiveAlbumartist().empty() && song.album().empty() && song.title().empty())) {
    callback({}, "No artist, album, or title");
    return;
  }
  network->Get(SearchUrl(song.EffectiveAlbumartist(), song.album(), song.title()), [callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({}, response.error.empty() ? "Deezer cover request failed" : response.error);
      return;
    }
    const std::vector<SearchResult> results = ParseResults(response.body);
    if (results.empty()) {
      callback({}, "No Deezer cover");
      return;
    }
    callback(results.front().image_url, {});
  });
}
