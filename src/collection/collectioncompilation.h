#ifndef STRAWBERRY_COLLECTIONCOMPILATION_H
#define STRAWBERRY_COLLECTIONCOMPILATION_H

#include "core/song.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace CollectionCompilation {

inline int Effective(bool compilation, bool detected, bool on, bool off) {
  return ((compilation || detected || on) && !off) ? 1 : 0;
}

inline std::vector<std::pair<std::string, std::string>> AlbumArtistKeys(const SongList &songs) {
  std::vector<std::pair<std::string, std::string>> keys;
  for (const Song &song : songs) {
    if (song.album().empty()) {
      continue;
    }
    const std::pair<std::string, std::string> key{song.album(), song.artist()};
    if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
      keys.push_back(key);
    }
  }
  return keys;
}

}  // namespace CollectionCompilation

#endif
