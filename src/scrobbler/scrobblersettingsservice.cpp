#include "scrobbler/scrobblersettingsservice.h"

#include "constants/scrobblersettings.h"
#include "core/settings.h"

void ScrobblerSettingsService::Reload() {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  enabled_ = settings.BoolValue(ScrobblerSettings::kEnabled, ScrobblerSettings::kDefaultEnabled);
  scrobble_offline_ = settings.BoolValue(ScrobblerSettings::kOffline, ScrobblerSettings::kDefaultOffline);
  submit_delay_ = settings.IntValue(ScrobblerSettings::kSubmit, ScrobblerSettings::kDefaultSubmit);
}
