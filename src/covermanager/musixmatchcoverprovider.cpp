#include "covermanager/musixmatchcoverprovider.h"

#include "utilities/jsonutils.h"
#include "utilities/musixmatchprovider.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

std::string MusixmatchCoverProvider::AlbumUrl(const std::string &artist, const std::string &album) {
  return "https://www.musixmatch.com/album/" + MusixmatchProvider::StringFixup(artist) + "/" + MusixmatchProvider::StringFixup(album);
}

std::string MusixmatchCoverProvider::ExtractNextDataJson(const std::string &html) {
  const std::string begin = "<script id=\"__NEXT_DATA__\" type=\"application/json\">";
  const std::string end = "</script>";
  const auto start = html.find(begin);
  if (start == std::string::npos) {
    return {};
  }
  const auto json_start = start + begin.size();
  const auto stop = html.find(end, json_start);
  if (stop == std::string::npos || stop <= json_start) {
    return {};
  }
  const std::string json = html.substr(json_start, stop - json_start);
  if (json.find('<') != std::string::npos) {
    return {};
  }
  return json;
}

std::vector<MusixmatchCoverProvider::SearchResult> MusixmatchCoverProvider::ParseAlbumPage(const std::string &html, const std::string &artist,
                                                                                           const std::string &album) {
  std::vector<SearchResult> results;
  const std::string json = ExtractNextDataJson(html);
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
  const char *path[] = {"props", "pageProps", "data", "albumGet", "data"};
  for (const char *key : path) {
    if (!json_object_has_member(object, key) || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, key))) {
      g_object_unref(parser);
      return results;
    }
    object = json_object_get_object_member(object, key);
  }
  const char *artist_name = json_object_has_member(object, "artistName") ? json_object_get_string_member(object, "artistName") : nullptr;
  const char *name = json_object_has_member(object, "name") ? json_object_get_string_member(object, "name") : nullptr;
  const std::string result_artist = artist_name ? artist_name : "";
  const std::string result_album = name ? name : "";
  if (StrUtils::ToLower(result_artist) != StrUtils::ToLower(artist) && StrUtils::ToLower(result_album) != StrUtils::ToLower(album)) {
    g_object_unref(parser);
    return results;
  }
  SearchResult result;
  result.artist = result_artist;
  result.album = result_album;
  for (const char *key : {"coverImage800x800", "coverImage500x500", "coverImage350x350"}) {
    if (!json_object_has_member(object, key)) {
      continue;
    }
    const char *url = json_object_get_string_member(object, key);
    if (url && *url) {
      result.image_url = url;
      results.push_back(result);
      break;
    }
  }
  g_object_unref(parser);
  return results;
}

void MusixmatchCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || song.EffectiveAlbumartist().empty() || song.album().empty()) {
    callback({}, "No artist or album");
    return;
  }
  const std::string artist = song.EffectiveAlbumartist();
  const std::string album = song.album();
  network->Get(AlbumUrl(artist, album), [callback, artist, album](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({}, response.error.empty() ? "Musixmatch cover request failed" : response.error);
      return;
    }
    const std::vector<SearchResult> results = ParseAlbumPage(response.body, artist, album);
    if (results.empty()) {
      callback({}, "No Musixmatch cover");
      return;
    }
    callback(results.front().image_url, {});
  });
}
