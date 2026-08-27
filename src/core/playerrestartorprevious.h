#ifndef STRAWBERRY_PLAYERRESTARTORPREVIOUS_H
#define STRAWBERRY_PLAYERRESTARTORPREVIOUS_H

#include <cstdint>

namespace PlayerRestartOrPrevious {

inline constexpr int kThresholdSec = 8;
inline constexpr int64_t kNsecPerSec = 1000000000LL;

// Qt Player::RestartOrPrevious goes to the previous track when position < 8 s.
inline bool ShouldGoToPrevious(int64_t pos_ns, int threshold_sec = kThresholdSec) {
  return pos_ns < static_cast<int64_t>(threshold_sec) * kNsecPerSec;
}

}  // namespace PlayerRestartOrPrevious

#endif
