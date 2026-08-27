#ifndef STRAWBERRY_MUSICBRAINZDISCID_H
#define STRAWBERRY_MUSICBRAINZDISCID_H

#include "core/song.h"
#include "tagfetcher/musicbrainzclient.h"
#include "tagfetcher/tagfetchhelpers.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

#include <cstdlib>
#include <string>

namespace MusicBrainzDiscId {

inline constexpr char kDiscUrl[] = "https://musicbrainz.org/ws/2/discid/";

inline std::string DiscUrl(const std::string &disc_id) {
  return std::string(kDiscUrl) + StrUtils::UriEscape(disc_id) + "?inc=recordings+artists&fmt=json";
}

inline bool ShouldLookup(const std::string &disc_id) { return !disc_id.empty(); }

inline bool TitleIsGeneric(const std::string &title, int track) {
  return title.empty() || title == "Track " + std::to_string(track);
}

inline std::string ArtistCreditName(JsonArray *credits) {
  if (!credits) {
    return {};
  }
  std::string name;
  const guint n = json_array_get_length(credits);
  for (guint i = 0; i < n; ++i) {
    JsonObject *credit = json_array_get_object_element(credits, i);
    if (!credit) {
      continue;
    }
    std::string part;
    if (json_object_has_member(credit, "artist") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(credit, "artist"))) {
      JsonObject *artist = json_object_get_object_member(credit, "artist");
      if (artist && json_object_has_member(artist, "name")) {
        part = json_object_get_string_member(artist, "name");
      }
    }
    if (part.empty() && json_object_has_member(credit, "name")) {
      part = json_object_get_string_member(credit, "name");
    }
    std::string join;
    if (json_object_has_member(credit, "joinphrase")) {
      join = json_object_get_string_member(credit, "joinphrase");
    }
    name += part + join;
  }
  return name;
}

inline int ParseYear(const std::string &date) {
  if (date.size() < 4) {
    return -1;
  }
  if (date[0] != '1' && date[0] != '2') {
    return -1;
  }
  return std::atoi(date.substr(0, 4).c_str());
}

inline bool MediaHasDisc(JsonObject *media, const std::string &disc_id) {
  if (!media || disc_id.empty()) {
    return true;
  }
  if (!json_object_has_member(media, "discs") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(media, "discs"))) {
    return false;
  }
  JsonArray *discs = json_object_get_array_member(media, "discs");
  const guint n = json_array_get_length(discs);
  for (guint i = 0; i < n; ++i) {
    JsonObject *disc = json_array_get_object_element(discs, i);
    if (disc && json_object_has_member(disc, "id") && disc_id == json_object_get_string_member(disc, "id")) {
      return true;
    }
  }
  return false;
}

inline MusicBrainzClient::ResultList ParseDiscResults(const std::string &json, const std::string &disc_id) {
  MusicBrainzClient::ResultList results;
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
  if (json_array_get_length(releases) == 0) {
    g_object_unref(parser);
    return results;
  }
  // Qt DiscIdRequestFinished uses the first release only.
  JsonObject *release = json_array_get_object_element(releases, 0);
  if (!release) {
    g_object_unref(parser);
    return results;
  }
  const std::string album = json_object_has_member(release, "title") ? json_object_get_string_member(release, "title") : "";
  const std::string album_artist =
      json_object_has_member(release, "artist-credit") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(release, "artist-credit"))
          ? ArtistCreditName(json_object_get_array_member(release, "artist-credit"))
          : "";
  const int year = json_object_has_member(release, "date") ? ParseYear(json_object_get_string_member(release, "date")) : -1;
  const std::string album_id = json_object_has_member(release, "id") ? json_object_get_string_member(release, "id") : "";
  if (!json_object_has_member(release, "media") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(release, "media"))) {
    g_object_unref(parser);
    return results;
  }
  JsonArray *media_list = json_object_get_array_member(release, "media");
  const guint media_n = json_array_get_length(media_list);
  for (guint m = 0; m < media_n; ++m) {
    JsonObject *media = json_array_get_object_element(media_list, m);
    if (!MediaHasDisc(media, disc_id)) {
      continue;
    }
    if (!json_object_has_member(media, "tracks") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(media, "tracks"))) {
      continue;
    }
    JsonArray *tracks = json_object_get_array_member(media, "tracks");
    const guint track_n = json_array_get_length(tracks);
    for (guint t = 0; t < track_n; ++t) {
      JsonObject *track = json_array_get_object_element(tracks, t);
      if (!track) {
        continue;
      }
      MusicBrainzClient::Result result;
      result.album = album;
      result.album_artist = album_artist;
      result.year = year;
      result.musicbrainz_album_id = album_id;
      result.track = json_object_has_member(track, "position") ? json_object_get_int_member(track, "position") : 0;
      result.title = json_object_has_member(track, "title") ? json_object_get_string_member(track, "title") : "";
      result.duration_msec = json_object_has_member(track, "length") ? json_object_get_int_member(track, "length") : 0;
      if (json_object_has_member(track, "artist-credit") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(track, "artist-credit"))) {
        result.artist = ArtistCreditName(json_object_get_array_member(track, "artist-credit"));
      }
      if (json_object_has_member(track, "recording") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(track, "recording"))) {
        JsonObject *recording = json_object_get_object_member(track, "recording");
        if (recording && json_object_has_member(recording, "id")) {
          result.musicbrainz_recording_id = json_object_get_string_member(recording, "id");
        }
        if (result.title.empty() && recording && json_object_has_member(recording, "title")) {
          result.title = json_object_get_string_member(recording, "title");
        }
      }
      results.push_back(result);
    }
  }
  g_object_unref(parser);
  return results;
}

inline SongList MergeByTrack(const SongList &cdda, const MusicBrainzClient::ResultList &results) {
  SongList merged = cdda;
  for (Song &song : merged) {
    for (const auto &result : results) {
      if (result.track > 0 && result.track == song.track()) {
        song = TagFetchHelpers::ApplyResult(song, result);
        if (!song.musicbrainz_disc_id().empty()) {
          break;
        }
        break;
      }
    }
  }
  return merged;
}

inline std::string DiscIdFromSongs(const SongList &songs) {
  for (const Song &song : songs) {
    if (!song.musicbrainz_disc_id().empty()) {
      return song.musicbrainz_disc_id();
    }
  }
  return {};
}

}  // namespace MusicBrainzDiscId

#endif
