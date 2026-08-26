#include "scrobbler/scrobblersettingsservice.h"

#include "core/settings.h"

void ScrobblerSettingsService::Reload() {
  Settings settings;
  settings.BeginGroup("Scrobbler");
  enabled_ = settings.BoolValue("enabled", false);
  scrobble_offline_ = settings.BoolValue("offline", true);
  submit_delay_ = settings.IntValue("submit_delay", 0);
}
