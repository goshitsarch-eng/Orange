#ifndef STRAWBERRY_COLLECTIONSCANPROGRESS_H
#define STRAWBERRY_COLLECTIONSCANPROGRESS_H

#include "core/song.h"

#include <algorithm>
#include <string>
#include <vector>

namespace CollectionScanProgress {

inline int Percent(int done, int total) {
  if (total <= 0) {
    return 0;
  }
  return std::clamp(done * 100 / total, 0, 100);
}

inline bool ShouldReport(int done) { return done > 0 && (done % 25) == 0; }

inline int Total(int songs) { return std::max(0, songs); }

inline int CountAudioPaths(const std::vector<std::string> &paths) {
  int count = 0;
  for (const std::string &path : paths) {
    if (Song::IsAudioFile(path)) {
      ++count;
    }
  }
  return count;
}

}  // namespace CollectionScanProgress

#endif  // STRAWBERRY_COLLECTIONSCANPROGRESS_H
