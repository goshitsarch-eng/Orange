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

// Qt Playlist::stop_after_current: stop_after row equals the row that just finished.
inline bool ShouldStopAfterRow(int stop_after_row, int current_row) {
  return stop_after_row >= 0 && stop_after_row == current_row;
}

inline bool ShouldStopAfterTrack(PlaylistSequence::RepeatMode mode, bool stop_after_current, int stop_after_row, int current_row) {
  return ShouldStopAfterTrack(mode, stop_after_current) || ShouldStopAfterRow(stop_after_row, current_row);
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
