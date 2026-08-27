#ifndef STRAWBERRY_PLAYERRESUME_H
#define STRAWBERRY_PLAYERRESUME_H

#include "engine/enginebase.h"

namespace PlayerResume {

constexpr char kSettingsGroup[] = "Player";
constexpr char kPlaybackState[] = "playback_state";
constexpr char kPlaybackPlaylist[] = "playback_playlist";
constexpr char kPlaybackPosition[] = "playback_position";

inline bool IsResumableState(int state) {
  return state == static_cast<int>(EngineBase::State::Playing) || state == static_cast<int>(EngineBase::State::Paused);
}

inline bool ShouldResume(bool enabled, int state) { return enabled && IsResumableState(state); }

inline bool ShouldPause(int state) { return state == static_cast<int>(EngineBase::State::Paused); }

inline int64_t PositionToNanosec(int64_t seconds) { return seconds < 0 ? 0 : seconds * 1000000000LL; }

}  // namespace PlayerResume

#endif
