#ifndef STRAWBERRY_COLLECTIONSONGPATCH_H
#define STRAWBERRY_COLLECTIONSONGPATCH_H

#include "core/song.h"

namespace CollectionSongPatch {

inline bool PatchById(SongList *songs, const Song &updated) {
  if (!songs || updated.id() <= 0) {
    return false;
  }
  bool changed = false;
  for (Song &song : *songs) {
    if (song.id() == updated.id()) {
      song = updated;
      changed = true;
    }
  }
  return changed;
}

inline int PatchAll(SongList *songs, const SongList &updated) {
  int count = 0;
  for (const Song &song : updated) {
    if (PatchById(songs, song)) {
      ++count;
    }
  }
  return count;
}

}  // namespace CollectionSongPatch

#endif  // STRAWBERRY_COLLECTIONSONGPATCH_H
