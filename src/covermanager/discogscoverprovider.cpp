#include "covermanager/discogscoverprovider.h"

#include "covermanager/albumcoverfetchersearch.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

const char *DiscogsCoverProvider::kUrlSearch = "https://api.discogs.com/database/search";
const char *DiscogsCoverProvider::kAccessKeyB64 = "dGh6ZnljUGJlZ1NEeXBuSFFxSVk=";
const char *DiscogsCoverProvider::kSecretKeyB64 = "ZkFIcmlaSER4aHhRSlF2U3d0bm5ZVmdxeXFLWUl0UXI=";

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

}  // namespace

std::string DiscogsCoverProvider::AccessKey() { return DecodeB64(kAccessKeyB64); }

std::string DiscogsCoverProvider::SecretKey() { return DecodeB64(kSecretKeyB64); }

std::string DiscogsCoverProvider::SearchUrl(const std::string &artist, const std::string &album, const std::string &type) {
  return std::string(kUrlSearch) + "?format=album&type=" + StrUtils::UriEscape(type) + "&artist=" + StrUtils::UriEscape(StrUtils::ToLower(artist)) +
         "&release_title=" + StrUtils::UriEscape(StrUtils::ToLower(album)) + "&key=" + StrUtils::UriEscape(AccessKey()) +
         "&secret=" + StrUtils::UriEscape(SecretKey());
}

bool DiscogsCoverProvider::AcceptImage(int width, int height) {
  if (width < 300 || height < 300) {
    return false;
  }
  const float aspect = 1.0f - static_cast<float>(std::max(width, height) - std::min(width, height)) / static_cast<float>(std::max(width, height));
  return aspect >= 0.85f;
}

std::vector<DiscogsCoverProvider::SearchHit> DiscogsCoverProvider::ParseSearchResults(const std::string &json, const std::string &artist,
                                                                                     const std::string &album) {
  std::vector<SearchHit> hits;
  if (json.empty()) {
    return hits;
  }
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return hits;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return hits;
  }
  JsonObject *object = json_node_get_object(root);
  if (!json_object_has_member(object, "results") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "results"))) {
    g_object_unref(parser);
    return hits;
  }
  JsonArray *array = json_object_get_array_member(object, "results");
  const guint n = json_array_get_length(array);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item = json_array_get_element(array, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    JsonObject *result = json_node_get_object(item);
    const std::string title = ObjectString(result, "title");
    const std::string resource_url = ObjectString(result, "resource_url");
    const std::string id = ObjectString(result, "id");
    if (title.empty() || resource_url.empty()) {
      continue;
    }
    if (title.find(" - ") != std::string::npos) {
      const auto split = title.find(" - ");
      const std::string title_artist = title.substr(0, split);
      const std::string title_album = title.substr(split + 3);
      if (StrUtils::ToLower(title_artist) != StrUtils::ToLower(artist) && StrUtils::ToLower(title_album) != StrUtils::ToLower(album)) {
        continue;
      }
    }
    SearchHit hit;
    hit.title = title;
    hit.resource_url = resource_url;
    hit.id = id;
    hits.push_back(hit);
  }
  g_object_unref(parser);
  return hits;
}

std::vector<DiscogsCoverProvider::ImageResult> DiscogsCoverProvider::ParseReleaseImages(const std::string &json, const std::string &search_artist,
                                                                                        const std::string &search_album) {
  std::vector<ImageResult> images;
  if (json.empty()) {
    return images;
  }
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return images;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return images;
  }
  JsonObject *object = json_node_get_object(root);
  if (!json_object_has_member(object, "artists") || !json_object_has_member(object, "title") || !json_object_has_member(object, "images") ||
      !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "artists")) || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "images"))) {
    g_object_unref(parser);
    return images;
  }
  JsonArray *artists = json_object_get_array_member(object, "artists");
  std::string artist;
  int artist_count = 0;
  const guint artist_n = json_array_get_length(artists);
  for (guint i = 0; i < artist_n; ++i) {
    JsonNode *item = json_array_get_element(artists, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    artist = ObjectString(json_node_get_object(item), "name");
    ++artist_count;
    if (artist == search_artist) {
      break;
    }
  }
  if (artist.empty()) {
    g_object_unref(parser);
    return images;
  }
  if (artist_count > 1 && artist != search_artist) {
    artist = "Various artists";
  }
  const std::string album = ObjectString(object, "title");
  if (artist != search_artist && album != search_album) {
    g_object_unref(parser);
    return images;
  }
  JsonArray *array = json_object_get_array_member(object, "images");
  const guint n = json_array_get_length(array);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item = json_array_get_element(array, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    JsonObject *image = json_node_get_object(item);
    if (ObjectString(image, "type") != "primary") {
      continue;
    }
    const int width = ObjectInt(image, "width");
    const int height = ObjectInt(image, "height");
    if (!AcceptImage(width, height)) {
      continue;
    }
    const std::string url = ObjectString(image, "resource_url");
    if (url.empty()) {
      continue;
    }
    ImageResult result;
    result.artist = artist;
    result.album = album;
    result.image_url = url;
    images.push_back(result);
  }
  g_object_unref(parser);
  return images;
}

namespace {

void LoadDiscogsHits(NetworkAccessManager *network, const std::string &provider, const std::string &artist, const std::string &album,
                     std::vector<DiscogsCoverProvider::SearchHit> hits, size_t index, CoverProviderSearchResults collected,
                     CoverProvider::SearchCallback callback) {
  if (index >= hits.size()) {
    callback(collected);
    return;
  }
  network->Get(hits[index].resource_url, [network, provider, artist, album, hits, index, collected, callback](const NetworkAccessManager::Response &release) {
    CoverProviderSearchResults next = collected;
    if (release.ok()) {
      for (const DiscogsCoverProvider::ImageResult &image : DiscogsCoverProvider::ParseReleaseImages(release.body, artist, album)) {
        next.push_back(AlbumCoverFetcherSearch::FromHit(provider, image.artist, image.album, image.image_url));
      }
    }
    LoadDiscogsHits(network, provider, artist, album, hits, index + 1, next, callback);
  });
}

}  // namespace

void DiscogsCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  Search(song, network, [callback](const CoverProviderSearchResults &results) {
    if (results.empty()) {
      callback({}, "No Discogs cover");
      return;
    }
    callback(results.front().image_url, {});
  });
}

void DiscogsCoverProvider::Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) {
  if (!network || song.EffectiveAlbumartist().empty() || song.album().empty()) {
    callback({});
    return;
  }
  const std::string artist = song.EffectiveAlbumartist();
  const std::string album = song.album();
  const std::string provider = name();
  network->Get(SearchUrl(artist, album, "master"), [network, callback, artist, album, provider](const NetworkAccessManager::Response &response) {
    auto start_hits = [network, callback, artist, album, provider](const std::vector<SearchHit> &hits) {
      if (hits.empty()) {
        callback({});
        return;
      }
      LoadDiscogsHits(network, provider, artist, album, hits, 0, {}, callback);
    };
    if (response.ok()) {
      const std::vector<SearchHit> hits = ParseSearchResults(response.body, artist, album);
      if (!hits.empty()) {
        start_hits(hits);
        return;
      }
    }
    network->Get(SearchUrl(artist, album, "release"), [start_hits, artist, album](const NetworkAccessManager::Response &release_search) {
      if (!release_search.ok()) {
        start_hits({});
        return;
      }
      start_hits(ParseSearchResults(release_search.body, artist, album));
    });
  });
}
