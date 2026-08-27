#ifndef STRAWBERRY_STREAMINGCOLLECTIONSTORE_H
#define STRAWBERRY_STREAMINGCOLLECTIONSTORE_H

#include "core/song.h"

#include <cstring>
#include <string>
#include <vector>

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

inline bool CanStore(const std::string &service, List list) { return ValidTable(TableName(service, list)); }

inline const char *AddLabel(List list) {
  switch (list) {
    case List::Artists:
      return "Add to artists";
    case List::Albums:
      return "Add to albums";
    case List::Songs:
    default:
      return "Add to songs";
  }
}

inline const char *AddedStatus(List list) {
  switch (list) {
    case List::Artists:
      return "Added to artists";
    case List::Albums:
      return "Added to albums";
    case List::Songs:
    default:
      return "Added to songs";
  }
}

inline std::vector<List> AddableLists(const std::string &service) {
  std::vector<List> lists;
  for (List list : {List::Artists, List::Albums, List::Songs}) {
    if (CanStore(service, list)) {
      lists.push_back(list);
    }
  }
  return lists;
}

inline SongList MergeSongs(const SongList &existing, const SongList &added) {
  SongList out = existing;
  for (const Song &song : added) {
    const std::string key = PersistUrl(song);
    if (key.empty()) {
      continue;
    }
    bool found = false;
    for (Song &have : out) {
      if (PersistUrl(have) == key) {
        have = song;
        found = true;
        break;
      }
    }
    if (!found) {
      out.push_back(song);
    }
  }
  return out;
}

inline int AddedCount(const SongList &existing, const SongList &merged) {
  if (merged.size() <= existing.size()) {
    return 0;
  }
  return static_cast<int>(merged.size() - existing.size());
}

// Qt StreamingCollectionView::RemoveSelectedSongs → CollectionBackend::DeleteSongs.
inline bool SamePersistKey(const Song &left, const Song &right) {
  const std::string key = PersistUrl(left);
  return !key.empty() && key == PersistUrl(right);
}

inline SongList SubtractSongs(const SongList &existing, const SongList &removed) {
  SongList out;
  for (const Song &have : existing) {
    bool drop = false;
    for (const Song &song : removed) {
      if (SamePersistKey(have, song)) {
        drop = true;
        break;
      }
    }
    if (!drop) {
      out.push_back(have);
    }
  }
  return out;
}

inline int RemovedCount(const SongList &existing, const SongList &remaining) {
  if (remaining.size() >= existing.size()) {
    return 0;
  }
  return static_cast<int>(existing.size() - remaining.size());
}

inline const char *RemovedStatus(List list) {
  switch (list) {
    case List::Artists:
      return "Removed from artists";
    case List::Albums:
      return "Removed from albums";
    case List::Songs:
    default:
      return "Removed from songs";
  }
}

// Artists/Albums/Songs tabs persist a local catalogue; Favorites/Search do not.
inline bool ListFromTab(const char *tab, List *list) {
  if (!tab || !list) {
    return false;
  }
  if (std::strcmp(tab, "artists") == 0) {
    *list = List::Artists;
    return true;
  }
  if (std::strcmp(tab, "albums") == 0) {
    *list = List::Albums;
    return true;
  }
  if (std::strcmp(tab, "songs") == 0) {
    *list = List::Songs;
    return true;
  }
  return false;
}

SongList Load(Database *database, const std::string &table);
void Replace(Database *database, const std::string &table, const SongList &songs);
int Merge(Database *database, const std::string &table, const SongList &songs);
int Remove(Database *database, const std::string &table, const SongList &songs);

}  // namespace StreamingCollectionStore

#endif
