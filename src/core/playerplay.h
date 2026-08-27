#ifndef STRAWBERRY_PLAYERPLAY_H
#define STRAWBERRY_PLAYERPLAY_H

#include "engine/enginebase.h"

#include <cstdint>

namespace PlayerPlay {

enum class Action { Seek, UnPause, Start };

// Qt Player::Play: Playing seeks, Paused unpauses, otherwise starts the current playlist row.
inline Action ForState(EngineBase::State state) {
  switch (state) {
    case EngineBase::State::Playing:
      return Action::Seek;
    case EngineBase::State::Paused:
      return Action::UnPause;
    default:
      return Action::Start;
  }
}

// Qt Player::Play(offset_nanosec) while Playing calls SeekTo(offset_nanosec).
// SeekTo interprets the argument as seconds, so the default Play() seeks to 0.
inline int64_t SeekSeconds(uint64_t offset_nanosec) { return static_cast<int64_t>(offset_nanosec); }

}  // namespace PlayerPlay

#endif  // STRAWBERRY_PLAYERPLAY_H
