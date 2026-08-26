#ifndef STRAWBERRY_STREAMINGCOLLECTIONSTORE_H
#define STRAWBERRY_STREAMINGCOLLECTIONSTORE_H

#include "core/song.h"

#include <string>

class Database;

namespace StreamingCollectionStore {

enum class List { Artists = 1, Albums = 2, Songs = 3 };

inline std::string Prefix(const std::string &service) {
  if (service == "Tidal") {
    return "tidal";
  }
  if (service == "Qobuz") {
    return "qobuz";
  }
  if (service == "Spotify") {
    return "spotify";
  }
  if (service == "Subsonic") {
    return "subsonic";
  }
  return {};
}

inline bool HasCache(const std::string &service) { return !Prefix(service).empty(); }

inline std::string TableName(const std::string &service, List list) {
  const std::string prefix = Prefix(service);
  if (prefix.empty()) {
    return {};
  }
  if (prefix == "subsonic") {
    return list == List::Songs ? "subsonic_songs" : std::string();
  }
  switch (list) {
    case List::Artists:
      return prefix + "_artists_songs";
    case List::Albums:
      return prefix + "_albums_songs";
    case List::Songs:
      return prefix + "_songs";
    default:
      return {};
  }
}

inline bool ValidTable(const std::string &table) {
  return table == "tidal_artists_songs" || table == "tidal_albums_songs" || table == "tidal_songs" || table == "qobuz_artists_songs" ||
         table == "qobuz_albums_songs" || table == "qobuz_songs" || table == "spotify_artists_songs" || table == "spotify_albums_songs" ||
         table == "spotify_songs" || table == "subsonic_songs";
}

inline std::string PersistUrl(const Song &song) {
  if (!song.url().empty()) {
    return song.url();
  }
  if (!song.song_id().empty()) {
    return song.song_id();
  }
  if (!song.album_id().empty()) {
    return "album:" + song.album_id();
  }
  if (!song.artist_id().empty()) {
    return "artist:" + song.artist_id();
  }
  return {};
}

inline bool ShouldPersist(bool has_error, bool logged_in, const SongList &songs) {
  if (has_error) {
    return false;
  }
  return !songs.empty() || logged_in;
}

inline bool ShouldKeepCache(bool has_error, const SongList &songs) { return has_error && songs.empty(); }

SongList Load(Database *database, const std::string &table);
void Replace(Database *database, const std::string &table, const SongList &songs);

}  // namespace StreamingCollectionStore

#endif
