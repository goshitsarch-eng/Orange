#include "spotify/spotifymetadatarequest.h"

#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

#include <cstdint>
#include <cstdlib>

namespace {

constexpr int64_t kNsecPerMsec = 1000000LL;

}  // namespace

namespace SpotifyMetadataRequest {

std::string TrackUrl(const std::string &api_url, const std::string &track_id) {
  return api_url + "/tracks/" + StrUtils::UriEscape(track_id);
}

std::string ArtistUrl(const std::string &api_url, const std::string &artist_id) {
  return api_url + "/artists/" + StrUtils::UriEscape(artist_id);
}

Song ParseTrack(const std::string &json) {
  Song song(Song::Source::Spotify);
  const std::string id = JsonUtils::GetString(json, {"id"});
  if (id.empty()) {
    return song;
  }
  song.set_valid(true);
  song.set_song_id(id);
  const std::string uri = JsonUtils::GetString(json, {"uri"});
  song.set_url(uri.empty() ? "spotify://" + id : uri);
  song.set_title(JsonUtils::GetString(json, {"name"}));
  song.set_track(JsonUtils::GetInt(json, {"track_number"}, -1));
  song.set_disc(JsonUtils::GetInt(json, {"disc_number"}, -1));
  const int duration_ms = JsonUtils::GetInt(json, {"duration_ms"}, -1);
  if (duration_ms > 0) {
    song.set_length_nanosec(static_cast<int64_t>(duration_ms) * kNsecPerMsec);
  }
  song.set_artist(JsonUtils::GetString(json, {"artists", "name"}));
  song.set_artist_id(JsonUtils::GetString(json, {"artists", "id"}));
  song.set_album(JsonUtils::GetString(json, {"album", "name"}));
  song.set_album_id(JsonUtils::GetString(json, {"album", "id"}));
  song.set_albumartist(JsonUtils::GetString(json, {"album", "artists", "name"}));
  const std::string release = JsonUtils::GetString(json, {"album", "release_date"});
  if (release.size() >= 4) {
    song.set_year(std::atoi(release.substr(0, 4).c_str()));
  }
  const std::string preview = JsonUtils::GetString(json, {"preview_url"});
  if (!preview.empty()) {
    song.set_stream_url(preview);
  }

  JsonParser *parser = json_parser_new();
  if (json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    JsonNode *root = json_parser_get_root(parser);
    if (root && JSON_NODE_HOLDS_OBJECT(root)) {
      JsonObject *object = json_node_get_object(root);
      if (json_object_has_member(object, "artists") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "artists"))) {
        JsonArray *artists = json_object_get_array_member(object, "artists");
        if (json_array_get_length(artists) > 0) {
          JsonObject *artist = json_array_get_object_element(artists, 0);
          if (song.artist().empty()) {
            song.set_artist(json_object_has_member(artist, "name") ? json_object_get_string_member(artist, "name") : "");
          }
          if (song.artist_id().empty()) {
            song.set_artist_id(json_object_has_member(artist, "id") ? json_object_get_string_member(artist, "id") : "");
          }
        }
      }
      if (json_object_has_member(object, "album") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, "album"))) {
        JsonObject *album = json_object_get_object_member(object, "album");
        if (json_object_has_member(album, "images") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(album, "images"))) {
          JsonArray *images = json_object_get_array_member(album, "images");
          const guint n = json_array_get_length(images);
          for (guint i = 0; i < n; ++i) {
            JsonObject *image = json_array_get_object_element(images, i);
            const int width = json_object_has_member(image, "width") ? static_cast<int>(json_object_get_int_member(image, "width")) : 0;
            const int height = json_object_has_member(image, "height") ? static_cast<int>(json_object_get_int_member(image, "height")) : 0;
            const char *url = json_object_has_member(image, "url") ? json_object_get_string_member(image, "url") : nullptr;
            if (url && width >= 300 && height >= 300) {
              song.set_art_automatic(url);
              break;
            }
          }
          if (song.art_automatic().empty() && n > 0) {
            JsonObject *image = json_array_get_object_element(images, 0);
            if (json_object_has_member(image, "url")) {
              song.set_art_automatic(json_object_get_string_member(image, "url"));
            }
          }
        }
      }
    }
  }
  g_object_unref(parser);
  return song;
}

std::string ParseArtistGenre(const std::string &json) {
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return {};
  }
  std::string genre;
  JsonNode *root = json_parser_get_root(parser);
  if (root && JSON_NODE_HOLDS_OBJECT(root)) {
    JsonObject *object = json_node_get_object(root);
    if (json_object_has_member(object, "genres") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "genres"))) {
      JsonArray *genres = json_object_get_array_member(object, "genres");
      if (json_array_get_length(genres) > 0) {
        JsonNode *first = json_array_get_element(genres, 0);
        if (first && JSON_NODE_HOLDS_VALUE(first) && json_node_get_value_type(first) == G_TYPE_STRING) {
          const char *value = json_node_get_string(first);
          if (value) {
            genre = value;
          }
        }
      }
    }
  }
  g_object_unref(parser);
  return genre;
}

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers, Callback callback) {
  if (!network || url.empty()) {
    if (callback) {
      callback(Song(), "Spotify metadata request is incomplete");
    }
    return;
  }
  network->Get(
      url,
      [callback](const NetworkAccessManager::Response &response) {
        if (!callback) {
          return;
        }
        if (!response.ok()) {
          callback(Song(), response.error.empty() ? "Spotify metadata missing" : response.error);
          return;
        }
        const Song song = ParseTrack(response.body);
        callback(song, song.is_valid() ? std::string() : "Spotify metadata missing");
      },
      headers);
}

}  // namespace SpotifyMetadataRequest
