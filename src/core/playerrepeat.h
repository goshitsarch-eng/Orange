#ifndef STRAWBERRY_PLAYERREPEAT_H
#define STRAWBERRY_PLAYERREPEAT_H

#include "playlist/playlistsequence.h"

#include <algorithm>
#include <cstdint>

namespace PlayerRepeat {

constexpr int64_t kIntroNanosec = 10 * 1000000000LL;

inline bool ShouldStopAfterTrack(PlaylistSequence::RepeatMode mode, bool stop_after_current) {
  return stop_after_current || mode == PlaylistSequence::RepeatMode::OneByOne;
}

inline bool IsIntro(PlaylistSequence::RepeatMode mode) { return mode == PlaylistSequence::RepeatMode::Intro; }

inline bool IntroElapsed(int64_t position_nanosec, int64_t intro_nanosec = kIntroNanosec) {
  return position_nanosec >= intro_nanosec;
}

inline unsigned IntroTimeoutMs(int64_t intro_nanosec = kIntroNanosec) {
  return static_cast<unsigned>(std::max<int64_t>(1, intro_nanosec / 1000000));
}

}  // namespace PlayerRepeat

#endif
