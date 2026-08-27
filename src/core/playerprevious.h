#ifndef STRAWBERRY_PLAYERPREVIOUS_H
#define STRAWBERRY_PLAYERPREVIOUS_H

#include "constants/behavioursettings.h"

#include <cstdint>

namespace PlayerPrevious {

inline bool ShouldRestartTrack(BehaviourSettings::PreviousBehaviour mode, int64_t last_press_sec, int64_t now_sec,
                               int threshold_sec = 2) {
  if (mode != BehaviourSettings::PreviousBehaviour::Restart) {
    return false;
  }
  if (last_press_sec <= 0) {
    return true;
  }
  return now_sec - last_press_sec >= threshold_sec;
}

inline bool ShouldSeekToStart(BehaviourSettings::PreviousBehaviour mode, int64_t pos_ns, int threshold_sec = 3) {
  return mode == BehaviourSettings::PreviousBehaviour::DontRestart && pos_ns > static_cast<int64_t>(threshold_sec) * 1000000000LL;
}

}  // namespace PlayerPrevious

#endif  // STRAWBERRY_PLAYERPREVIOUS_H
