#ifndef STRAWBERRY_PLAYERSEEKNOTIFY_H
#define STRAWBERRY_PLAYERSEEKNOTIFY_H

#include <algorithm>
#include <cstdint>

namespace PlayerSeekNotify {

inline int64_t Clamp(int64_t seek_ns, int64_t length_ns) {
  if (seek_ns < 0) {
    return 0;
  }
  if (length_ns > 0) {
    return std::min(seek_ns, length_ns);
  }
  return seek_ns;
}

inline bool ShouldRefreshNowPlaying(int64_t seek_ns, int64_t length_ns) { return seek_ns == 0 && length_ns > 0; }

}  // namespace PlayerSeekNotify

#endif  // STRAWBERRY_PLAYERSEEKNOTIFY_H
