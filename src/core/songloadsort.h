#ifndef STRAWBERRY_SONGLOADSORT_H
#define STRAWBERRY_SONGLOADSORT_H

#include "core/song.h"

#include <algorithm>

namespace SongLoadSort {

// Qt SongLoader::CompareSongs: artist, album, disc, track, then URL.
inline bool LessThan(const Song &left, const Song &right) {
  if (left.artist() < right.artist()) {
    return true;
  }
  if (left.artist() > right.artist()) {
    return false;
  }
  if (left.album() < right.album()) {
    return true;
  }
  if (left.album() > right.album()) {
    return false;
  }
  if (left.disc() < right.disc()) {
    return true;
  }
  if (left.disc() > right.disc()) {
    return false;
  }
  if (left.track() < right.track()) {
    return true;
  }
  if (left.track() > right.track()) {
    return false;
  }
  return left.url() < right.url();
}

inline void StableSort(SongList &songs) { std::stable_sort(songs.begin(), songs.end(), LessThan); }

}  // namespace SongLoadSort

#endif
