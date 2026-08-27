#ifndef STRAWBERRY_WINSMTCSTATUS_H
#define STRAWBERRY_WINSMTCSTATUS_H

#include "engine/enginebase.h"

#include <string>

namespace WinSmtcStatus {

enum class Playback { Closed, Changing, Stopped, Playing, Paused };

inline Playback FromEngine(EngineBase::State state) {
  switch (state) {
    case EngineBase::State::Playing:
      return Playback::Playing;
    case EngineBase::State::Paused:
      return Playback::Paused;
    case EngineBase::State::Error:
      return Playback::Closed;
    case EngineBase::State::Empty:
    case EngineBase::State::Idle:
    default:
      return Playback::Stopped;
  }
}

inline bool ButtonsEnabled(Playback playback) { return playback == Playback::Playing || playback == Playback::Paused; }

inline bool TimelineEnabled(Playback playback) { return playback != Playback::Closed && playback != Playback::Stopped; }

}  // namespace WinSmtcStatus

#endif
