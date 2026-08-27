#ifndef STRAWBERRY_SCROBBLERERROR_H
#define STRAWBERRY_SCROBBLERERROR_H

#include "constants/scrobblersettings.h"
#include "core/settings.h"

#include <string>

namespace ScrobblerError {

inline bool ShouldShowDialog(bool setting_enabled, const std::string &message) { return setting_enabled && !message.empty(); }

inline bool ShouldShowDialog(const std::string &message) {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  return ShouldShowDialog(settings.BoolValue(ScrobblerSettings::kShowErrorDialog, ScrobblerSettings::kDefaultShowErrorDialog), message);
}

inline std::string RequestFailed(const std::string &service) { return service + ": request failed"; }

inline std::string NotAuthenticated(const std::string &service) { return service + ": not authenticated"; }

}  // namespace ScrobblerError

#endif
