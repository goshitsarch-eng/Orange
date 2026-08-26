#ifndef STRAWBERRY_NOTIFICATIONSCONTROLS_H
#define STRAWBERRY_NOTIFICATIONSCONTROLS_H

#include "constants/notificationssettings.h"
#include "settings/notificationssettingslabels.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

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

inline bool TypeEnabled(OSDSettings::Type type, bool native, bool tray, bool pretty) {
  switch (type) {
    case OSDSettings::Type::Disabled:
      return true;
    case OSDSettings::Type::Native:
      return native;
    case OSDSettings::Type::TrayPopup:
      return tray;
    case OSDSettings::Type::Pretty:
      return pretty;
  }
  return false;
}

inline OSDSettings::Type EffectiveType(OSDSettings::Type requested, bool native, bool tray, bool pretty) {
  if (TypeEnabled(requested, native, tray, pretty)) {
    return requested;
  }
  if (pretty) {
    return OSDSettings::Type::Pretty;
  }
  if (native) {
    return OSDSettings::Type::Native;
  }
  if (tray) {
    return OSDSettings::Type::TrayPopup;
  }
  return OSDSettings::Type::Disabled;
}

inline OSDSettings::Type EffectiveType(int requested, bool native, bool tray, bool pretty) {
  return EffectiveType(static_cast<OSDSettings::Type>(requested), native, tray, pretty);
}

inline bool GeneralSensitive(OSDSettings::Type type) { return type != OSDSettings::Type::Disabled; }

inline bool PrettyGroupSensitive(OSDSettings::Type type) { return type == OSDSettings::Type::Pretty; }

inline bool CustomTextSensitive(OSDSettings::Type type) { return type != OSDSettings::Type::Disabled; }

inline bool ArtSensitive(OSDSettings::Type type) { return type != OSDSettings::Type::Disabled && type != OSDSettings::Type::TrayPopup; }

inline bool DisableDurationSensitive(OSDSettings::Type type) { return type == OSDSettings::Type::Pretty; }

inline OSDSettings::Type TypeFromId(const std::string &id) {
  return static_cast<OSDSettings::Type>(std::atoi(id.c_str()));
}

inline OSDSettings::Type TypeFromSelected(const std::vector<std::string> *ids, unsigned index) {
  if (!ids || index >= ids->size()) {
    return OSDSettings::Type::Disabled;
  }
  return TypeFromId((*ids)[index]);
}

inline std::vector<std::pair<std::string, const char *>> AvailableTypes(bool native, bool tray, bool pretty) {
  std::vector<std::pair<std::string, const char *>> types = {{"0", NotificationsSettingsLabels::Disabled()}};
  if (native) {
    types.emplace_back("1", NotificationsSettingsLabels::Native());
  }
  if (tray) {
    types.emplace_back("2", NotificationsSettingsLabels::TrayPopup());
  }
  if (pretty) {
    types.emplace_back("3", NotificationsSettingsLabels::Pretty());
  }
  return types;
}

}  // namespace NotificationsControls

#endif
