#ifndef STRAWBERRY_OSDFORCE_H
#define STRAWBERRY_OSDFORCE_H

#include "constants/notificationssettings.h"

namespace OSDForce {

// Qt ShowPlaying: Disabled + force_show_next_ falls through to Pretty.
inline bool ShouldShowWhenDisabled(bool force_show_next) { return force_show_next; }

inline bool ConsumeForce(bool *force_show_next) {
  if (!force_show_next || !*force_show_next) {
    return false;
  }
  *force_show_next = false;
  return true;
}

inline OSDSettings::Type TypeAfterForce(OSDSettings::Type type, bool force_show_next) {
  if (type == OSDSettings::Type::Disabled && force_show_next) {
    return OSDSettings::Type::Pretty;
  }
  return type;
}

}  // namespace OSDForce

#endif
