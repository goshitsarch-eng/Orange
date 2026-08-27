#ifndef STRAWBERRY_COLLECTIONUNAVAILABLE_H
#define STRAWBERRY_COLLECTIONUNAVAILABLE_H

#include "core/song.h"

#include <algorithm>
#include <string>
#include <vector>

namespace CollectionUnavailable {

// Qt CollectionBackend::MarkSongsUnavailable: missing URLs become unavailable and notify listeners.
inline bool IsMissing(const Song &song, const std::vector<std::string> &seen_urls) {
  if (song.unavailable()) {
    return false;
  }
  return std::find(seen_urls.begin(), seen_urls.end(), song.url()) == seen_urls.end();
}

inline Song MarkedCopy(const Song &song) {
  Song copy = song;
  copy.set_unavailable(true);
  return copy;
}

inline bool ShouldNotify(const SongList &marked) { return !marked.empty(); }

}  // namespace CollectionUnavailable

#endif
