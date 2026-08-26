#ifndef STRAWBERRY_PLAYERREPEAT_H
#define STRAWBERRY_PLAYERREPEAT_H

#include "playlist/playlistsequence.h"

namespace PlayerRepeat {

inline bool ShouldStopAfterTrack(PlaylistSequence::RepeatMode mode, bool stop_after_current) {
  return stop_after_current || mode == PlaylistSequence::RepeatMode::OneByOne;
}

}  // namespace PlayerRepeat

#endif
