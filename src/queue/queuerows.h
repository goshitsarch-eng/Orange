#ifndef STRAWBERRY_QUEUEROWS_H
#define STRAWBERRY_QUEUEROWS_H

#include <algorithm>
#include <vector>

namespace QueueRows {

struct Source {
  int playlist_id = -1;
  int row = -1;
  bool valid() const { return playlist_id >= 0 && row >= 0; }
};

inline int PositionForRow(const std::vector<Source> &sources, int playlist_id, int row) {
  if (playlist_id < 0 || row < 0) {
    return 0;
  }
  for (size_t i = 0; i < sources.size(); ++i) {
    if (sources[i].playlist_id == playlist_id && sources[i].row == row) {
      return static_cast<int>(i) + 1;
    }
  }
  return 0;
}

inline std::vector<Source> AfterRemove(const std::vector<Source> &sources, int playlist_id, const std::vector<int> &removed) {
  std::vector<int> sorted = removed;
  std::sort(sorted.begin(), sorted.end());
  sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
  std::vector<Source> out;
  out.reserve(sources.size());
  for (Source source : sources) {
    if (!source.valid() || source.playlist_id != playlist_id) {
      out.push_back(source);
      continue;
    }
    if (std::binary_search(sorted.begin(), sorted.end(), source.row)) {
      continue;
    }
    int shift = 0;
    for (int row : sorted) {
      if (row < source.row) {
        ++shift;
      }
    }
    source.row -= shift;
    out.push_back(source);
  }
  return out;
}

inline std::vector<Source> AfterInsert(const std::vector<Source> &sources, int playlist_id, int at, int count) {
  if (playlist_id < 0 || count <= 0) {
    return sources;
  }
  std::vector<Source> out = sources;
  for (Source &source : out) {
    if (source.valid() && source.playlist_id == playlist_id && source.row >= at) {
      source.row += count;
    }
  }
  return out;
}

}  // namespace QueueRows

#endif
