#ifndef STRAWBERRY_COVERMANAGERSTATS_H
#define STRAWBERRY_COVERMANAGERSTATS_H

#include <string>

namespace CoverManagerStats {

inline const char *TotalLabel() { return "Total albums:"; }
inline const char *WithoutLabel() { return "Without cover:"; }
inline const char *FetchMissing() { return "Fetch Missing Covers"; }
inline const char *Export() { return "Export Covers"; }
inline const char *Load() { return "Load"; }
inline const char *AddToPlaylist() { return "Add to playlist"; }

inline int WithoutCover(int total, int with_cover) {
  if (total < 0) {
    total = 0;
  }
  if (with_cover < 0) {
    with_cover = 0;
  }
  return total > with_cover ? total - with_cover : 0;
}

inline std::string CountText(int count) { return std::to_string(count < 0 ? 0 : count); }

}  // namespace CoverManagerStats

#endif  // STRAWBERRY_COVERMANAGERSTATS_H
