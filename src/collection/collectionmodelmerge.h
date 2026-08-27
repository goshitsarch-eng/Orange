#ifndef STRAWBERRY_COLLECTIONMODELMERGE_H
#define STRAWBERRY_COLLECTIONMODELMERGE_H

#include "collection/collectionmodelupdate.h"
#include "core/song.h"

#include <algorithm>

namespace CollectionModelMerge {

inline int IndexOfId(const SongList &songs, int id) {
  if (id <= 0) {
    return -1;
  }
  for (int i = 0; i < static_cast<int>(songs.size()); ++i) {
    if (songs[static_cast<size_t>(i)].id() == id) {
      return i;
    }
  }
  return -1;
}

inline SongList Add(SongList songs, const SongList &incoming) {
  for (const Song &song : incoming) {
    const int existing = IndexOfId(songs, song.id());
    if (existing >= 0) {
      songs[static_cast<size_t>(existing)] = song;
    } else {
      songs.push_back(song);
    }
  }
  return songs;
}

inline SongList Remove(SongList songs, const SongList &incoming) {
  songs.erase(std::remove_if(songs.begin(), songs.end(),
                             [&incoming](const Song &song) {
                               return IndexOfId(incoming, song.id()) >= 0;
                             }),
              songs.end());
  return songs;
}

inline SongList Update(SongList songs, const SongList &incoming) {
  for (const Song &song : incoming) {
    const int existing = IndexOfId(songs, song.id());
    if (existing >= 0) {
      songs[static_cast<size_t>(existing)] = song;
    }
  }
  return songs;
}

inline SongList Apply(const SongList &songs, const CollectionModelUpdate &update) {
  switch (update.type) {
    case CollectionModelUpdateType::AddSongs:
      return Add(songs, update.songs);
    case CollectionModelUpdateType::RemoveSongs:
      return Remove(songs, update.songs);
    case CollectionModelUpdateType::UpdateSongs:
      return Update(songs, update.songs);
    case CollectionModelUpdateType::Reset:
    default:
      return update.songs;
  }
}

}  // namespace CollectionModelMerge

#endif  // STRAWBERRY_COLLECTIONMODELMERGE_H
