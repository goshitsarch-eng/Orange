#include "settings/waveformsettingspage.h"

#include "constants/waveformsettings.h"
#include "core/seekbarsettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *WaveformSettingsPage::Create(Settings *settings, Application *) {
  AdwPreferencesPage *page = SettingsPage::MakePage("Waveform", "weather-showers-symbolic");
  settings->BeginGroup(SeekbarSettings::kSettingsGroup);
  AdwPreferencesGroup *seek = SettingsPage::AddGroup(page, "Seek bar");
  SettingsPage::AddIntCombo(seek, settings, SeekbarSettings::kSettingsGroup, SeekbarSettings::kMode, "Mode",
                            {{"0", "Normal"}, {"1", "Moodbar"}, {"2", "Waveform"}},
                            static_cast<int>(SeekbarSettings::kDefaultMode));
  settings->BeginGroup(WaveformSettings::kSettingsGroup);
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Waveform");
  SettingsPage::AddColorButton(group, settings, WaveformSettings::kSettingsGroup, WaveformSettings::kColor, "Color",
                              WaveformSettings::kDefaultColor);
  SettingsPage::AddToggle(group, settings, WaveformSettings::kSave, "Save generated waveforms", nullptr, WaveformSettings::kDefaultSave);
  return page;
}
