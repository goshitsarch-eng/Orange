#include "covermanager/lastfmcoverprovider.h"

#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <glib.h>
#include <json-glib/json-glib.h>

#include <cctype>
#include <cstring>

const char *LastFmCoverProvider::kUrl = "https://ws.audioscrobbler.com/2.0/";
const char *LastFmCoverProvider::kApiKey = "211990b4c96782c05d1536e7219eb56e";
const char *LastFmCoverProvider::kSecret = "80fd738f49596e9709b1bf9319c444a8";

namespace {

std::string NodeString(JsonNode *node) {
  if (!node || !JSON_NODE_HOLDS_VALUE(node) || json_node_get_value_type(node) != G_TYPE_STRING) {
    return {};
  }
  const char *value = json_node_get_string(node);
  return value ? value : "";
}

std::string ObjectString(JsonObject *object, const char *name) {
  if (!object || !json_object_has_member(object, name)) {
    return {};
  }
  return NodeString(json_object_get_member(object, name));
}

}  // namespace

std::string LastFmCoverProvider::Sign(const std::map<std::string, std::string> &params) {
  std::string data;
  for (const auto &param : params) {
    data += param.first + param.second;
  }
  data += kSecret;
  gchar *digest = g_compute_checksum_for_string(G_CHECKSUM_MD5, data.c_str(), static_cast<gssize>(data.size()));
  std::string result = digest ? digest : "";
  g_free(digest);
  return result;
}

std::string LastFmCoverProvider::FormBody(const std::map<std::string, std::string> &params) {
  std::string body;
  for (const auto &param : params) {
    if (!body.empty()) {
      body += "&";
    }
    body += StrUtils::UriEscape(param.first) + "=" + StrUtils::UriEscape(param.second);
  }
  return body;
}

LastFmCoverProvider::ImageSize LastFmCoverProvider::ImageSizeFromString(const std::string &size) {
  const std::string lower = StrUtils::ToLower(size);
  if (lower == "small") return ImageSize::Small;
  if (lower == "medium") return ImageSize::Medium;
  if (lower == "large") return ImageSize::Large;
  if (lower == "extralarge") return ImageSize::ExtraLarge;
  return ImageSize::Unknown;
}

std::string LastFmCoverProvider::UpgradeImageUrl(const std::string &url) {
  return StrUtils::Replace(url, "/300x300/", "/740x0/");
}

std::vector<LastFmCoverProvider::SearchResult> LastFmCoverProvider::ParseResults(const std::string &json, const std::string &type) {
  std::vector<SearchResult> results;
  if (json.empty() || type.empty()) {
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
  JsonObject *root_object = json_node_get_object(root);
  if (!json_object_has_member(root_object, "results") || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(root_object, "results"))) {
    g_object_unref(parser);
    return results;
  }
  JsonObject *object_results = json_object_get_object_member(root_object, "results");
  const std::string matches_key = type == "track" ? "trackmatches" : "albummatches";
  if (!json_object_has_member(object_results, matches_key.c_str()) ||
      !JSON_NODE_HOLDS_OBJECT(json_object_get_member(object_results, matches_key.c_str()))) {
    g_object_unref(parser);
    return results;
  }
  JsonObject *matches = json_object_get_object_member(object_results, matches_key.c_str());
  if (!json_object_has_member(matches, type.c_str()) || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(matches, type.c_str()))) {
    g_object_unref(parser);
    return results;
  }
  JsonArray *array = json_object_get_array_member(matches, type.c_str());
  const guint n = json_array_get_length(array);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item = json_array_get_element(array, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    JsonObject *object = json_node_get_object(item);
    const std::string artist = ObjectString(object, "artist");
    const std::string name = ObjectString(object, "name");
    if (!json_object_has_member(object, "image") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "image"))) {
      continue;
    }
    JsonArray *images = json_object_get_array_member(object, "image");
    std::string image_url;
    ImageSize image_size = ImageSize::Unknown;
    const guint image_count = json_array_get_length(images);
    for (guint img = 0; img < image_count; ++img) {
      JsonNode *image_node = json_array_get_element(images, img);
      if (!image_node || !JSON_NODE_HOLDS_OBJECT(image_node)) {
        continue;
      }
      JsonObject *image = json_node_get_object(image_node);
      const std::string url = ObjectString(image, "#text");
      if (url.empty()) {
        continue;
      }
      const ImageSize size = ImageSizeFromString(ObjectString(image, "size"));
      if (image_url.empty() || size > image_size) {
        image_url = url;
        image_size = size;
      }
    }
    if (image_url.empty()) {
      continue;
    }
    SearchResult result;
    result.artist = artist;
    result.album = type == "album" ? name : std::string();
    result.image_url = UpgradeImageUrl(image_url);
    results.push_back(result);
  }
  g_object_unref(parser);
  return results;
}

void LastFmCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || (song.EffectiveAlbumartist().empty() && song.album().empty() && song.title().empty())) {
    callback({}, "No artist, album, or title");
    return;
  }
  std::string method;
  std::string type;
  std::string query = song.EffectiveAlbumartist();
  if (song.album().empty() && !song.title().empty()) {
    method = "track.search";
    type = "track";
    if (!query.empty()) {
      query += " ";
    }
    query += song.title();
  } else {
    method = "album.search";
    type = "album";
    if (!song.album().empty()) {
      if (!query.empty()) {
        query += " ";
      }
      query += song.album();
    }
  }
  std::string lang = "en";
  const char *const *langs = g_get_language_names();
  if (langs && langs[0] && std::strlen(langs[0]) >= 2) {
    lang.assign(langs[0], 2);
    for (char &ch : lang) {
      ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
  }
  std::map<std::string, std::string> params = {{"api_key", kApiKey}, {"lang", lang}, {"method", method}, {type, query}};
  params["api_sig"] = Sign(params);
  params["format"] = "json";
  network->Post(kUrl, FormBody(params), [callback, type](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({}, response.error.empty() ? "Last.fm cover request failed" : response.error);
      return;
    }
    const std::vector<SearchResult> results = ParseResults(response.body, type);
    if (results.empty()) {
      const std::string fallback = JsonUtils::FindCoverUrl(response.body);
      if (!fallback.empty()) {
        callback(UpgradeImageUrl(fallback), {});
        return;
      }
      callback({}, "No Last.fm cover");
      return;
    }
    callback(results.front().image_url, {});
  }, "application/x-www-form-urlencoded");
}
