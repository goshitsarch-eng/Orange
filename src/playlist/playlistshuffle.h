#ifndef STRAWBERRY_PLAYLISTSHUFFLE_H
#define STRAWBERRY_PLAYLISTSHUFFLE_H

#include "core/song.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace PlaylistShuffle {

inline std::string AlbumKey(const Song &song) { return song.EffectiveAlbumartist() + "\n" + song.album(); }

inline std::string GroupingKey(const Song &song) { return song.grouping().empty() ? AlbumKey(song) : song.grouping(); }

inline std::vector<int> Identity(int count) {
  std::vector<int> items(count > 0 ? static_cast<size_t>(count) : 0);
  std::iota(items.begin(), items.end(), 0);
  return items;
}

inline std::vector<int> ShuffleAll(int count, unsigned seed, int current = -1) {
  std::vector<int> items = Identity(count);
  std::mt19937 rng(seed);
  std::shuffle(items.begin(), items.end(), rng);
  if (current >= 0) {
    auto it = std::find(items.begin(), items.end(), current);
    if (it != items.end()) {
      items.erase(it);
      items.insert(items.begin(), current);
    }
  }
  return items;
}

inline std::vector<int> ShuffleByKey(const std::vector<std::string> &keys, unsigned seed, int first_row = -1) {
  const int count = static_cast<int>(keys.size());
  std::vector<int> items = Identity(count);
  std::vector<std::string> unique;
  for (const std::string &key : keys) {
    if (std::find(unique.begin(), unique.end(), key) == unique.end()) {
      unique.push_back(key);
    }
  }
  std::mt19937 rng(seed);
  std::shuffle(unique.begin(), unique.end(), rng);
  if (first_row >= 0 && first_row < count) {
    const std::string &first = keys[static_cast<size_t>(first_row)];
    auto it = std::find(unique.begin(), unique.end(), first);
    if (it != unique.begin() && it != unique.end()) {
      std::iter_swap(unique.begin(), it);
    }
  }
  std::map<std::string, int> position;
  for (int i = 0; i < static_cast<int>(unique.size()); ++i) {
    position[unique[static_cast<size_t>(i)]] = i;
  }
  std::stable_sort(items.begin(), items.end(), [&](int left, int right) {
    const int a = position[keys[static_cast<size_t>(left)]];
    const int b = position[keys[static_cast<size_t>(right)]];
    if (a != b) {
      return a < b;
    }
    return left < right;
  });
  return items;
}

}  // namespace PlaylistShuffle

#endif
