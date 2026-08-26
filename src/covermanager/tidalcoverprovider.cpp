#include "covermanager/tidalcoverprovider.h"

#include "core/settings.h"
#include "covermanager/albumcoverfetchersearch.h"
#include "tidal/tidalservice.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

const int TidalCoverProvider::kLimit = 10;

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

std::string TidalCoverProvider::ImageUrl(const std::string &cover, const std::string &size) {
  return std::string(TidalService::kResourcesUrl) + "/images/" + StrUtils::Replace(cover, "-", "/") + "/" + size + ".jpg";
}

std::string TidalCoverProvider::SearchUrl(const std::string &artist, const std::string &album, const std::string &title, const std::string &country) {
  std::string resource;
  std::string query = artist;
  if (album.empty() && !title.empty()) {
    resource = "search/tracks";
    if (!query.empty()) {
      query += " ";
    }
    query += title;
  } else {
    resource = "search/albums";
    if (!album.empty()) {
      if (!query.empty()) {
        query += " ";
      }
      query += album;
    }
  }
  return std::string(TidalService::kApiUrl) + "/" + resource + "?query=" + StrUtils::UriEscape(query) + "&limit=" + std::to_string(kLimit) +
         "&countryCode=" + StrUtils::UriEscape(country.empty() ? "US" : country);
}

std::vector<TidalCoverProvider::SearchResult> TidalCoverProvider::ParseItems(const std::string &json) {
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
  if (!json_object_has_member(object, "items") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "items"))) {
    g_object_unref(parser);
    return results;
  }
  JsonArray *items = json_object_get_array_member(object, "items");
  const guint n = json_array_get_length(items);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item_node = json_array_get_element(items, i);
    if (!item_node || !JSON_NODE_HOLDS_OBJECT(item_node)) {
      continue;
    }
    JsonObject *item = json_node_get_object(item_node);
    if (!json_object_has_member(item, "artist") || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(item, "artist"))) {
      continue;
    }
    const std::string artist = ObjectString(json_object_get_object_member(item, "artist"), "name");
    JsonObject *album = item;
    if (json_object_has_member(item, "album") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(item, "album"))) {
      album = json_object_get_object_member(item, "album");
    }
    const std::string title = ObjectString(album, "title");
    const std::string cover = ObjectString(album, "cover");
    if (title.empty() || cover.empty()) {
      continue;
    }
    SearchResult result;
    result.artist = artist;
    result.album = Song::AlbumRemoveDiscMisc(title);
    result.image_url = ImageUrl(cover, "1280x1280");
    results.push_back(result);
  }
  g_object_unref(parser);
  return results;
}

void TidalCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  Search(song, network, [callback](const CoverProviderSearchResults &results) {
    if (results.empty()) {
      callback({}, "No Tidal cover");
      return;
    }
    callback(results.front().image_url, {});
  });
}

void TidalCoverProvider::Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) {
  if (!network || (song.EffectiveAlbumartist().empty() && song.album().empty() && song.title().empty())) {
    callback({});
    return;
  }
  Settings settings;
  settings.BeginGroup("Tidal");
  const std::string token = settings.Value("token").empty() ? settings.Value("access_token") : settings.Value("token");
  const std::string country = settings.Value("countrycode", "US");
  if (token.empty()) {
    callback({});
    return;
  }
  network->Get(SearchUrl(song.EffectiveAlbumartist(), song.album(), song.title(), country),
               [this, callback](const NetworkAccessManager::Response &response) {
                 CoverProviderSearchResults results;
                 if (!response.ok()) {
                   callback(results);
                   return;
                 }
                 for (const SearchResult &hit : ParseItems(response.body)) {
                   results.push_back(AlbumCoverFetcherSearch::FromHit(name(), hit.artist, hit.album, hit.image_url));
                 }
                 callback(results);
               },
               {{"Authorization", "Bearer " + token}});
}
