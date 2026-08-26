#ifndef STRAWBERRY_NOTIFICATIONSCONTROLS_H
#define STRAWBERRY_NOTIFICATIONSCONTROLS_H

#include "constants/notificationssettings.h"

#include <algorithm>

namespace NotificationsControls {

inline int MinSeconds() { return 1; }
inline int MaxSeconds() { return 20; }

inline int SecondsFromMs(int ms) { return std::clamp(ms / 1000, MinSeconds(), MaxSeconds()); }
inline int MsFromSeconds(int seconds) { return std::clamp(seconds, MinSeconds(), MaxSeconds()) * 1000; }

inline bool DurationSpinSensitive(OSDSettings::Type type, bool disable_duration) {
  if (type == OSDSettings::Type::Disabled) {
    return false;
  }
  if (type == OSDSettings::Type::Pretty && disable_duration) {
    return false;
  }
  return true;
}

inline bool DurationSpinSensitive(int type, bool disable_duration) {
  return DurationSpinSensitive(static_cast<OSDSettings::Type>(type), disable_duration);
}

}  // namespace NotificationsControls

#endif
