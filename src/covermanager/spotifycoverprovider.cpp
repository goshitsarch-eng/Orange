#include "covermanager/spotifycoverprovider.h"

#include "core/settings.h"
#include "covermanager/albumcoverfetchersearch.h"
#include "covermanager/coverproviderauth.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

const char *SpotifyCoverProvider::kApiUrl = "https://api.spotify.com/v1";
const int SpotifyCoverProvider::kLimit = 10;

bool SpotifyCoverProvider::authenticated() const { return CoverProviderAuth::HasServiceToken(name()); }

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

int ObjectInt(JsonObject *object, const char *name) {
  if (!object || !json_object_has_member(object, name) || !JSON_NODE_HOLDS_VALUE(json_object_get_member(object, name))) {
    return 0;
  }
  JsonNode *node = json_object_get_member(object, name);
  if (json_node_get_value_type(node) == G_TYPE_INT64) {
    return static_cast<int>(json_node_get_int(node));
  }
  return 0;
}

}  // namespace

std::string SpotifyCoverProvider::SearchUrl(const std::string &artist, const std::string &album, const std::string &title) {
  std::string type;
  std::string query = artist;
  if (album.empty() && !title.empty()) {
    type = "track";
    if (!query.empty()) {
      query += " ";
    }
    query += title;
  } else {
    type = "album";
    if (!album.empty()) {
      if (!query.empty()) {
        query += " ";
      }
      query += album;
    }
  }
  return std::string(kApiUrl) + "/search?q=" + StrUtils::UriEscape(query) + "&type=" + type + "&limit=" + std::to_string(kLimit);
}

std::vector<SpotifyCoverProvider::SearchResult> SpotifyCoverProvider::ParseResults(const std::string &json, const std::string &extract) {
  std::vector<SearchResult> results;
  if (json.empty() || extract.empty()) {
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
  if (!json_object_has_member(object, extract.c_str()) || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, extract.c_str()))) {
    g_object_unref(parser);
    return results;
  }
  JsonObject *group = json_object_get_object_member(object, extract.c_str());
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
    if (!json_object_has_member(album, "artists") || !json_object_has_member(album, "images") ||
        !JSON_NODE_HOLDS_ARRAY(json_object_get_member(album, "artists")) || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(album, "images"))) {
      continue;
    }
    const std::string album_name = ObjectString(album, "name");
    std::string artist;
    JsonArray *artists = json_object_get_array_member(album, "artists");
    if (json_array_get_length(artists) > 0 && JSON_NODE_HOLDS_OBJECT(json_array_get_element(artists, 0))) {
      artist = ObjectString(json_array_get_object_element(artists, 0), "name");
    }
    JsonArray *images = json_object_get_array_member(album, "images");
    const guint image_n = json_array_get_length(images);
    for (guint img = 0; img < image_n; ++img) {
      JsonNode *image_node = json_array_get_element(images, img);
      if (!image_node || !JSON_NODE_HOLDS_OBJECT(image_node)) {
        continue;
      }
      JsonObject *image = json_node_get_object(image_node);
      const int width = ObjectInt(image, "width");
      const int height = ObjectInt(image, "height");
      const std::string url = ObjectString(image, "url");
      if (url.empty() || width < 300 || height < 300) {
        continue;
      }
      SearchResult result;
      result.artist = artist;
      result.album = album_name;
      result.image_url = url;
      result.width = width;
      result.height = height;
      results.push_back(result);
    }
  }
  g_object_unref(parser);
  return results;
}

void SpotifyCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  Search(song, network, [callback](const CoverProviderSearchResults &results) {
    if (results.empty()) {
      callback({}, "No Spotify cover");
      return;
    }
    callback(results.front().image_url, {});
  });
}

void SpotifyCoverProvider::Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) {
  if (!network || (song.EffectiveAlbumartist().empty() && song.album().empty() && song.title().empty())) {
    callback({});
    return;
  }
  Settings settings;
  settings.BeginGroup("Spotify");
  const std::string token = settings.Value("token").empty() ? settings.Value("access_token") : settings.Value("token");
  if (token.empty()) {
    callback({});
    return;
  }
  const std::string extract = song.album().empty() && !song.title().empty() ? "tracks" : "albums";
  network->Get(SearchUrl(song.EffectiveAlbumartist(), song.album(), song.title()),
               [this, callback, extract](const NetworkAccessManager::Response &response) {
                 CoverProviderSearchResults results;
                 if (!response.ok()) {
                   callback(results);
                   return;
                 }
                 for (const SearchResult &hit : ParseResults(response.body, extract)) {
                   results.push_back(AlbumCoverFetcherSearch::FromHit(name(), hit.artist, hit.album, hit.image_url, hit.width, hit.height));
                 }
                 callback(results);
               },
               {{"Authorization", "Bearer " + token}});
}
