#include "covermanager/opentidalcoverprovider.h"

#include "core/logging.h"
#include "core/oauthenticator.h"
#include "core/settings.h"
#include "covermanager/albumcoverfetchersearch.h"
#include "utilities/strutils.h"

#include <glib.h>
#include <json-glib/json-glib.h>

#include <cstdlib>
#include <ctime>
#include <map>

const char *OpenTidalCoverProvider::kSettingsGroup = "OpenTidal";
const char *OpenTidalCoverProvider::kOAuthAccessTokenUrl = "https://auth.tidal.com/v1/oauth2/token";
const char *OpenTidalCoverProvider::kApiUrl = "https://openapi.tidal.com/v2";
const char *OpenTidalCoverProvider::kApiClientIdB64 = "RHBwV3FpTEM4ZFJSV1RJaQ==";
const char *OpenTidalCoverProvider::kApiClientSecretB64 = "cGk0QmxpclZXQWlteWpBc0RnWmZ5RmVlRzA2b3E1blVBVTljUW1IdFhDST0=";
const char *OpenTidalCoverProvider::kContentTypeHeader = "application/vnd.api+json";
const int OpenTidalCoverProvider::kSearchLimit = 6;
const int OpenTidalCoverProvider::kMinImageSize = 640;

namespace {

std::string DecodeB64(const char *b64) {
  gsize length = 0;
  guchar *bytes = g_base64_decode(b64, &length);
  std::string result(reinterpret_cast<char *>(bytes), length);
  g_free(bytes);
  return result;
}

std::string ObjectString(JsonObject *object, const char *name) {
  if (!object || !json_object_has_member(object, name) || !JSON_NODE_HOLDS_VALUE(json_object_get_member(object, name))) {
    return {};
  }
  if (json_node_get_value_type(json_object_get_member(object, name)) != G_TYPE_STRING) {
    if (json_node_get_value_type(json_object_get_member(object, name)) == G_TYPE_INT64) {
      return std::to_string(json_node_get_int(json_object_get_member(object, name)));
    }
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
  if (json_node_get_value_type(node) == G_TYPE_DOUBLE) {
    return static_cast<int>(json_node_get_double(node));
  }
  return std::atoi(ObjectString(object, name).c_str());
}

gint64 ObjectInt64(JsonObject *object, const char *name) {
  if (!object || !json_object_has_member(object, name) || !JSON_NODE_HOLDS_VALUE(json_object_get_member(object, name))) {
    return 0;
  }
  JsonNode *node = json_object_get_member(object, name);
  if (json_node_get_value_type(node) == G_TYPE_INT64) {
    return json_node_get_int(node);
  }
  if (json_node_get_value_type(node) == G_TYPE_DOUBLE) {
    return static_cast<gint64>(json_node_get_double(node));
  }
  return std::atoll(ObjectString(object, name).c_str());
}

JsonParser *LoadJson(const std::string &json) {
  if (json.empty()) {
    return nullptr;
  }
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return nullptr;
  }
  return parser;
}

std::map<std::string, std::string> ApiHeaders(const std::string &token) {
  return {{"Authorization", OpenTidalCoverProvider::AuthorizationHeader(token)},
          {"Content-Type", OpenTidalCoverProvider::kContentTypeHeader},
          {"Accept", OpenTidalCoverProvider::kContentTypeHeader}};
}

}  // namespace

std::string OpenTidalCoverProvider::ClientId() { return DecodeB64(kApiClientIdB64); }

std::string OpenTidalCoverProvider::ClientSecret() { return DecodeB64(kApiClientSecretB64); }

std::string OpenTidalCoverProvider::SearchQuery(const std::string &artist, const std::string &album, const std::string &title) {
  std::string query = artist;
  if (!album.empty()) {
    if (!query.empty()) {
      query += " ";
    }
    query += album;
  } else if (!title.empty()) {
    if (!query.empty()) {
      query += " ";
    }
    query += title;
  }
  return query;
}

std::string OpenTidalCoverProvider::SearchUrl(const std::string &artist, const std::string &album, const std::string &title, const std::string &country) {
  return std::string(kApiUrl) + "/searchResults/" + StrUtils::UriEscape(SearchQuery(artist, album, title)) +
         "?countryCode=" + StrUtils::UriEscape(country.empty() ? "US" : country) + "&limit=" + std::to_string(kSearchLimit) + "&include=albums";
}

std::string OpenTidalCoverProvider::CoverArtUrl(const std::string &album_id, const std::string &country) {
  return std::string(kApiUrl) + "/albums/" + StrUtils::UriEscape(album_id) +
         "/relationships/coverArt?countryCode=" + StrUtils::UriEscape(country.empty() ? "US" : country);
}

std::string OpenTidalCoverProvider::ArtworkUrl(const std::string &artwork_id, const std::string &country) {
  return std::string(kApiUrl) + "/artworks/" + StrUtils::UriEscape(artwork_id) +
         "?countryCode=" + StrUtils::UriEscape(country.empty() ? "US" : country);
}

std::string OpenTidalCoverProvider::AuthorizationHeader(const std::string &access_token) { return "Bearer " + access_token; }

bool OpenTidalCoverProvider::AcceptImage(int width, int height) { return width >= kMinImageSize && height >= kMinImageSize; }

std::vector<OpenTidalCoverProvider::AlbumHit> OpenTidalCoverProvider::ParseSearchAlbums(const std::string &json) {
  std::vector<AlbumHit> albums;
  JsonParser *parser = LoadJson(json);
  if (!parser) {
    return albums;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return albums;
  }
  JsonObject *object = json_node_get_object(root);
  if (!json_object_has_member(object, "included") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "included"))) {
    g_object_unref(parser);
    return albums;
  }
  JsonArray *included = json_object_get_array_member(object, "included");
  const guint n = json_array_get_length(included);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item_node = json_array_get_element(included, i);
    if (!item_node || !JSON_NODE_HOLDS_OBJECT(item_node)) {
      continue;
    }
    JsonObject *item = json_node_get_object(item_node);
    if (ObjectString(item, "type") != "albums") {
      continue;
    }
    AlbumHit hit;
    hit.id = ObjectString(item, "id");
    if (json_object_has_member(item, "attributes") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(item, "attributes"))) {
      hit.title = ObjectString(json_object_get_object_member(item, "attributes"), "title");
    }
    if (hit.id.empty()) {
      continue;
    }
    albums.push_back(hit);
  }
  g_object_unref(parser);
  return albums;
}

std::vector<std::string> OpenTidalCoverProvider::ParseCoverArtIds(const std::string &json) {
  std::vector<std::string> ids;
  JsonParser *parser = LoadJson(json);
  if (!parser) {
    return ids;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return ids;
  }
  JsonObject *object = json_node_get_object(root);
  if (!json_object_has_member(object, "data") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "data"))) {
    g_object_unref(parser);
    return ids;
  }
  JsonArray *data = json_object_get_array_member(object, "data");
  const guint n = json_array_get_length(data);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item_node = json_array_get_element(data, i);
    if (!item_node || !JSON_NODE_HOLDS_OBJECT(item_node)) {
      continue;
    }
    JsonObject *item = json_node_get_object(item_node);
    if (ObjectString(item, "type") != "artworks") {
      continue;
    }
    const std::string id = ObjectString(item, "id");
    if (!id.empty()) {
      ids.push_back(id);
    }
  }
  g_object_unref(parser);
  return ids;
}

std::vector<OpenTidalCoverProvider::ArtworkFile> OpenTidalCoverProvider::ParseArtworkFiles(const std::string &json) {
  std::vector<ArtworkFile> files;
  JsonParser *parser = LoadJson(json);
  if (!parser) {
    return files;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return files;
  }
  JsonObject *object = json_node_get_object(root);
  if (!json_object_has_member(object, "data") || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, "data"))) {
    g_object_unref(parser);
    return files;
  }
  JsonObject *data = json_object_get_object_member(object, "data");
  if (!json_object_has_member(data, "attributes") || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(data, "attributes"))) {
    g_object_unref(parser);
    return files;
  }
  JsonObject *attributes = json_object_get_object_member(data, "attributes");
  if (!json_object_has_member(attributes, "files") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(attributes, "files"))) {
    g_object_unref(parser);
    return files;
  }
  JsonArray *array = json_object_get_array_member(attributes, "files");
  const guint n = json_array_get_length(array);
  for (guint i = 0; i < n; ++i) {
    JsonNode *file_node = json_array_get_element(array, i);
    if (!file_node || !JSON_NODE_HOLDS_OBJECT(file_node)) {
      continue;
    }
    JsonObject *file = json_node_get_object(file_node);
    const std::string href = ObjectString(file, "href");
    if (href.empty() || !json_object_has_member(file, "meta") || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(file, "meta"))) {
      continue;
    }
    JsonObject *meta = json_object_get_object_member(file, "meta");
    ArtworkFile result;
    result.href = href;
    result.width = ObjectInt(meta, "width");
    result.height = ObjectInt(meta, "height");
    if (AcceptImage(result.width, result.height)) {
      files.push_back(result);
    }
  }
  g_object_unref(parser);
  return files;
}

std::vector<OpenTidalCoverProvider::SearchResult> OpenTidalCoverProvider::ResultsFromFiles(const std::string &artist, const std::string &album,
                                                                                          const std::vector<ArtworkFile> &files) {
  std::vector<SearchResult> results;
  for (const ArtworkFile &file : files) {
    SearchResult result;
    result.artist = artist;
    result.album = album;
    result.image_url = file.href;
    result.width = file.width;
    result.height = file.height;
    results.push_back(result);
  }
  return results;
}

OpenTidalCoverProvider::Token OpenTidalCoverProvider::ParseToken(const std::string &json) {
  Token token;
  JsonParser *parser = LoadJson(json);
  if (!parser) {
    return token;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return token;
  }
  JsonObject *object = json_node_get_object(root);
  token.access_token = ObjectString(object, "access_token");
  token.token_type = ObjectString(object, "token_type");
  token.expires_in = ObjectInt64(object, "expires_in");
  g_object_unref(parser);
  return token;
}

OpenTidalCoverProvider::ApiError OpenTidalCoverProvider::ParseApiError(const std::string &json) {
  ApiError error;
  JsonParser *parser = LoadJson(json);
  if (!parser) {
    return error;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return error;
  }
  JsonObject *object = json_node_get_object(root);
  if (!json_object_has_member(object, "errors") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "errors"))) {
    g_object_unref(parser);
    return error;
  }
  JsonArray *errors = json_object_get_array_member(object, "errors");
  const guint n = json_array_get_length(errors);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item_node = json_array_get_element(errors, i);
    if (!item_node || !JSON_NODE_HOLDS_OBJECT(item_node)) {
      continue;
    }
    JsonObject *item = json_node_get_object(item_node);
    error.category = ObjectString(item, "category");
    error.code = ObjectString(item, "code");
    error.detail = ObjectString(item, "detail");
    if (error.category == "AUTHENTICATION_ERROR") {
      error.authentication_error = true;
    }
    if (!error.detail.empty()) {
      break;
    }
  }
  g_object_unref(parser);
  return error;
}

void OpenTidalCoverProvider::EnsureToken(NetworkAccessManager *network, const std::function<void(const std::string &token, const std::string &error)> &callback) {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  const std::string stored = settings.Value("access_token");
  const std::string token_type = settings.Value("token_type", "Bearer");
  const gint64 expires_in = settings.Int64Value("expires_in");
  const gint64 login_time = settings.Int64Value("login_time");
  const gint64 now = static_cast<gint64>(std::time(nullptr));
  if (!stored.empty() && (expires_in <= 0 || login_time <= 0 || login_time + expires_in - 60 > now)) {
    callback(stored, {});
    return;
  }
  OAuthenticator oauth(network);
  oauth.ClientCredentials(kOAuthAccessTokenUrl, ClientId(), ClientSecret(), [callback](const std::string &body, const std::string &error) {
    if (!error.empty()) {
      callback({}, error);
      return;
    }
    const Token token = ParseToken(body);
    if (token.access_token.empty()) {
      callback({}, "OpenTidal token missing");
      return;
    }
    Settings store;
    store.BeginGroup(kSettingsGroup);
    store.SetValue("access_token", token.access_token);
    store.SetValue("token_type", token.token_type.empty() ? "Bearer" : token.token_type);
    store.SetInt64Value("expires_in", token.expires_in);
    store.SetInt64Value("login_time", static_cast<gint64>(std::time(nullptr)));
    store.Sync();
    callback(token.access_token, {});
    (void)token.token_type;
  });
}

void OpenTidalCoverProvider::SearchAlbums(NetworkAccessManager *network, const std::string &token, const Song &song, Callback callback) {
  const std::string artist = song.EffectiveAlbumartist();
  network->Get(SearchUrl(artist, song.album(), song.title()),
               [this, network, token, artist, callback](const NetworkAccessManager::Response &response) {
                 if (!response.ok()) {
                   const ApiError api_error = ParseApiError(response.body);
                   if (api_error.authentication_error) {
                     Settings settings;
                     settings.BeginGroup(kSettingsGroup);
                     settings.Remove("access_token");
                     settings.Sync();
                   }
                   callback({}, response.error.empty() ? (api_error.detail.empty() ? "OpenTidal search failed" : api_error.detail) : response.error);
                   return;
                 }
                 std::vector<AlbumHit> albums = ParseSearchAlbums(response.body);
                 if (albums.empty()) {
                   callback({}, "No OpenTidal album");
                   return;
                 }
                 FetchAlbumCovers(network, token, artist, std::move(albums), callback);
               },
               ApiHeaders(token));
}

void OpenTidalCoverProvider::FetchAlbumCovers(NetworkAccessManager *network, const std::string &token, const std::string &artist,
                                             std::vector<AlbumHit> albums, Callback callback) {
  if (albums.empty()) {
    callback({}, "No OpenTidal cover");
    return;
  }
  const AlbumHit album = albums.front();
  albums.erase(albums.begin());
  network->Get(CoverArtUrl(album.id),
               [this, network, token, artist, album, albums, callback](const NetworkAccessManager::Response &response) {
                 if (!response.ok()) {
                   FetchAlbumCovers(network, token, artist, albums, callback);
                   return;
                 }
                 std::vector<std::string> artwork_ids = ParseCoverArtIds(response.body);
                 if (artwork_ids.empty()) {
                   FetchAlbumCovers(network, token, artist, albums, callback);
                   return;
                 }
                 FetchArtworks(network, token, artist, album, std::move(artwork_ids), albums, callback);
               },
               ApiHeaders(token));
}

void OpenTidalCoverProvider::FetchArtworks(NetworkAccessManager *network, const std::string &token, const std::string &artist, const AlbumHit &album,
                                          std::vector<std::string> artwork_ids, std::vector<AlbumHit> remaining, Callback callback) {
  if (artwork_ids.empty()) {
    FetchAlbumCovers(network, token, artist, std::move(remaining), callback);
    return;
  }
  const std::string artwork_id = artwork_ids.front();
  artwork_ids.erase(artwork_ids.begin());
  network->Get(ArtworkUrl(artwork_id),
               [this, network, token, artist, album, artwork_ids, remaining, callback](const NetworkAccessManager::Response &response) {
                 if (response.ok()) {
                   const std::vector<ArtworkFile> files = ParseArtworkFiles(response.body);
                   if (!files.empty()) {
                     callback(files.front().href, {});
                     return;
                   }
                 }
                 FetchArtworks(network, token, artist, album, artwork_ids, remaining, callback);
               },
               ApiHeaders(token));
}

void OpenTidalCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || song.EffectiveAlbumartist().empty() || song.album().empty()) {
    callback({}, "No artist or album");
    return;
  }
  EnsureToken(network, [this, network, song, callback](const std::string &token, const std::string &error) {
    if (token.empty()) {
      callback({}, error.empty() ? "OpenTidal is not signed in" : error);
      return;
    }
    SearchAlbums(network, token, song, callback);
  });
}

void OpenTidalCoverProvider::Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) {
  if (!network || song.EffectiveAlbumartist().empty() || song.album().empty()) {
    callback({});
    return;
  }
  EnsureToken(network, [this, network, song, callback](const std::string &token, const std::string &) {
    if (token.empty()) {
      callback({});
      return;
    }
    SearchAlbums(network, token, song, callback);
  });
}

void OpenTidalCoverProvider::SearchAlbums(NetworkAccessManager *network, const std::string &token, const Song &song, SearchCallback callback) {
  const std::string artist = song.EffectiveAlbumartist();
  network->Get(SearchUrl(artist, song.album(), song.title()),
               [this, network, token, artist, callback](const NetworkAccessManager::Response &response) {
                 if (!response.ok()) {
                   callback({});
                   return;
                 }
                 std::vector<AlbumHit> albums = ParseSearchAlbums(response.body);
                 if (albums.empty()) {
                   callback({});
                   return;
                 }
                 SearchAlbumCovers(network, token, artist, std::move(albums), callback);
               },
               ApiHeaders(token));
}

void OpenTidalCoverProvider::SearchAlbumCovers(NetworkAccessManager *network, const std::string &token, const std::string &artist,
                                              std::vector<AlbumHit> albums, SearchCallback callback, CoverProviderSearchResults collected) {
  if (albums.empty()) {
    callback(collected);
    return;
  }
  const AlbumHit album = albums.front();
  albums.erase(albums.begin());
  network->Get(CoverArtUrl(album.id),
               [this, network, token, artist, album, albums, callback, collected](const NetworkAccessManager::Response &response) {
                 if (!response.ok()) {
                   SearchAlbumCovers(network, token, artist, albums, callback, collected);
                   return;
                 }
                 std::vector<std::string> artwork_ids = ParseCoverArtIds(response.body);
                 if (artwork_ids.empty()) {
                   SearchAlbumCovers(network, token, artist, albums, callback, collected);
                   return;
                 }
                 SearchArtworks(network, token, artist, album, std::move(artwork_ids), albums, callback, collected);
               },
               ApiHeaders(token));
}

void OpenTidalCoverProvider::SearchArtworks(NetworkAccessManager *network, const std::string &token, const std::string &artist, const AlbumHit &album,
                                           std::vector<std::string> artwork_ids, std::vector<AlbumHit> remaining, SearchCallback callback,
                                           CoverProviderSearchResults collected) {
  if (artwork_ids.empty()) {
    SearchAlbumCovers(network, token, artist, std::move(remaining), callback, collected);
    return;
  }
  const std::string artwork_id = artwork_ids.front();
  artwork_ids.erase(artwork_ids.begin());
  network->Get(ArtworkUrl(artwork_id),
               [this, network, token, artist, album, artwork_ids, remaining, callback, collected](const NetworkAccessManager::Response &response) {
                 CoverProviderSearchResults next = collected;
                 if (response.ok()) {
                   for (const SearchResult &hit : ResultsFromFiles(artist, album.title, ParseArtworkFiles(response.body))) {
                     next.push_back(AlbumCoverFetcherSearch::FromHit(name(), hit.artist, hit.album, hit.image_url, hit.width, hit.height));
                   }
                 }
                 SearchArtworks(network, token, artist, album, artwork_ids, remaining, callback, next);
               },
               ApiHeaders(token));
}
