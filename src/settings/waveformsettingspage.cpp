#include "settings/waveformsettingspage.h"

#include "constants/waveformsettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *WaveformSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(WaveformSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Waveform", "weather-showers-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Waveform");
  SettingsPage::AddToggle(group, settings, "enabled", "Show waveform seek bar", nullptr, true);
  SettingsPage::AddEntry(group, settings, WaveformSettings::kColor, "Color");
  SettingsPage::AddToggle(group, settings, WaveformSettings::kSave, "Save generated waveforms", nullptr, WaveformSettings::kDefaultSave);
  return page;
}
