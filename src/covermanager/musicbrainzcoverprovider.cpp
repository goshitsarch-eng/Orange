#include "covermanager/musicbrainzcoverprovider.h"

#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

const char *MusicbrainzCoverProvider::kReleaseSearchUrl = "https://musicbrainz.org/ws/2/release/";
const char *MusicbrainzCoverProvider::kAlbumCoverUrl = "https://coverartarchive.org/release/%s/front";
const int MusicbrainzCoverProvider::kLimit = 8;

std::string MusicbrainzCoverProvider::EscapeQuery(const std::string &value) {
  return StrUtils::Replace(StrUtils::Trim(value), "\"", "\\\"");
}

std::string MusicbrainzCoverProvider::CoverArtUrl(const std::string &release_id) {
  return "https://coverartarchive.org/release/" + release_id + "/front";
}

std::string MusicbrainzCoverProvider::SearchUrl(const std::string &artist, const std::string &album) {
  const std::string query = "release:\"" + EscapeQuery(album) + "\" AND artist:\"" + EscapeQuery(artist) + "\"";
  return std::string(kReleaseSearchUrl) + "?query=" + StrUtils::UriEscape(query) + "&limit=" + std::to_string(kLimit) + "&fmt=json";
}

std::vector<MusicbrainzCoverProvider::SearchResult> MusicbrainzCoverProvider::ParseReleases(const std::string &json, const std::string &search_artist) {
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
  if (!json_object_has_member(object, "releases") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "releases"))) {
    g_object_unref(parser);
    return results;
  }
  JsonArray *releases = json_object_get_array_member(object, "releases");
  const guint n = json_array_get_length(releases);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item = json_array_get_element(releases, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    JsonObject *release = json_node_get_object(item);
    if (!json_object_has_member(release, "id") || !json_object_has_member(release, "title") ||
        !json_object_has_member(release, "artist-credit")) {
      continue;
    }
    const char *id = json_object_get_string_member(release, "id");
    const char *title = json_object_get_string_member(release, "title");
    if (!id || !title || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(release, "artist-credit"))) {
      continue;
    }
    JsonArray *credits = json_object_get_array_member(release, "artist-credit");
    std::string artist;
    bool artist_matched = false;
    int credit_count = 0;
    const guint credit_n = json_array_get_length(credits);
    for (guint c = 0; c < credit_n; ++c) {
      JsonNode *credit_node = json_array_get_element(credits, c);
      if (!credit_node || !JSON_NODE_HOLDS_OBJECT(credit_node)) {
        continue;
      }
      JsonObject *credit = json_node_get_object(credit_node);
      if (!json_object_has_member(credit, "artist") || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(credit, "artist"))) {
        continue;
      }
      JsonObject *artist_obj = json_object_get_object_member(credit, "artist");
      const char *name = json_object_has_member(artist_obj, "name") ? json_object_get_string_member(artist_obj, "name") : nullptr;
      if (!name) {
        continue;
      }
      artist = name;
      ++credit_count;
      if (StrUtils::ToLower(artist) == StrUtils::ToLower(search_artist)) {
        artist_matched = true;
        break;
      }
    }
    if (credit_count > 1 && !artist_matched) {
      artist = "Various artists";
    }
    SearchResult result;
    result.artist = artist;
    result.album = title;
    result.release_id = id;
    result.image_url = CoverArtUrl(id);
    results.push_back(result);
  }
  g_object_unref(parser);
  return results;
}

void MusicbrainzCoverProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || song.EffectiveAlbumartist().empty() || song.album().empty()) {
    callback({}, "No artist or album");
    return;
  }
  const std::string artist = song.EffectiveAlbumartist();
  network->Get(SearchUrl(artist, song.album()), [callback, artist](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({}, response.error.empty() ? "MusicBrainz search failed" : response.error);
      return;
    }
    const std::vector<SearchResult> results = ParseReleases(response.body, artist);
    if (results.empty()) {
      callback({}, "No MusicBrainz release");
      return;
    }
    callback(results.front().image_url, {});
  });
}
