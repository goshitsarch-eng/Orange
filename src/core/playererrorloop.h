#ifndef STRAWBERRY_PLAYERERRORLOOP_H
#define STRAWBERRY_PLAYERERRORLOOP_H

#include "playlist/playlistsequence.h"

namespace PlayerErrorLoop {

inline constexpr int kRepeatTrackLimit = 3;
inline constexpr int kGlobalLimit = 100;

inline bool ShouldStopRepeatTrack(PlaylistSequence::RepeatMode mode, int errors) {
  return mode == PlaylistSequence::RepeatMode::Track && errors >= kRepeatTrackLimit;
}

inline bool ShouldStopAfterFilteredRows(int errors, int filtered_rows) { return filtered_rows > 0 && errors >= filtered_rows; }

inline bool ShouldStopGlobal(int errors) { return errors >= kGlobalLimit; }

inline bool ShouldStopAutoAdvance(PlaylistSequence::RepeatMode mode, int errors, int filtered_rows) {
  if (ShouldStopGlobal(errors)) {
    return true;
  }
  if (mode == PlaylistSequence::RepeatMode::Off) {
    return false;
  }
  return ShouldStopRepeatTrack(mode, errors) || ShouldStopAfterFilteredRows(errors, filtered_rows);
}

}  // namespace PlayerErrorLoop

#endif  // STRAWBERRY_PLAYERERRORLOOP_H
