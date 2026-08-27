#ifndef STRAWBERRY_SCROBBLEPOINT_H
#define STRAWBERRY_SCROBBLEPOINT_H

#include <algorithm>
#include <cstdint>

namespace ScrobblePoint {

constexpr int64_t kNsecPerSec = 1000000000LL;
constexpr int64_t kMinNsecs = 31LL * kNsecPerSec;
constexpr int64_t kMaxNsecs = 240LL * kNsecPerSec;

inline int64_t Compute(int64_t length_ns, int64_t seek_ns = 0) {
  if (seek_ns <= 0) {
    if (length_ns == 0) {
      return kMaxNsecs;
    }
    return std::clamp(length_ns / 2, kMinNsecs, kMaxNsecs);
  }
  if (length_ns <= 0) {
    return seek_ns + kMaxNsecs;
  }
  return std::clamp(seek_ns + (length_ns / 2), seek_ns + kMinNsecs, seek_ns + kMaxNsecs);
}

inline bool Reached(int64_t position_ns, int64_t point_ns) { return point_ns >= 0 && position_ns >= point_ns; }

inline bool ShouldSubmit(bool scrobbler_enabled, bool already_scrobbled, bool metadata_good, int64_t position_ns, int64_t point_ns) {
  return scrobbler_enabled && metadata_good && !already_scrobbled && Reached(position_ns, point_ns);
}

}  // namespace ScrobblePoint

#endif  // STRAWBERRY_SCROBBLEPOINT_H
