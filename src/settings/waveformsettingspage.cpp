#include "settings/waveformsettingspage.h"

#include "constants/waveformsettings.h"
#include "core/seekbarsettings.h"
#include "settings/settingspage.h"
#include "settings/waveformsettingslabels.h"
#include "widgets/seekbarmode.h"

AdwPreferencesPage *WaveformSettingsPage::Create(Settings *settings, Application *) {
  AdwPreferencesPage *page = SettingsPage::MakePage("Waveform", "weather-showers-symbolic");
  settings->BeginGroup(SeekbarSettings::kSettingsGroup);
  AdwPreferencesGroup *seek = SettingsPage::AddGroup(page, "Seek bar");
  SettingsPage::AddIntCombo(seek, settings, SeekbarSettings::kSettingsGroup, SeekbarSettings::kMode, "Mode",
                            {{"0", SeekbarModeMenu::Label(SeekbarSettings::Mode::Normal)},
                             {"1", SeekbarModeMenu::Label(SeekbarSettings::Mode::Moodbar)},
                             {"2", SeekbarModeMenu::Label(SeekbarSettings::Mode::Waveform)}},
                            static_cast<int>(SeekbarSettings::kDefaultMode));
  settings->BeginGroup(WaveformSettings::kSettingsGroup);
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Waveform");
  SettingsPage::AddColorButton(group, settings, WaveformSettings::kSettingsGroup, WaveformSettings::kColor, WaveformSettingsLabels::ColorTitle(),
                              WaveformSettings::kDefaultColor, WaveformSettingsLabels::ColorTooltip());
  SettingsPage::AddToggle(group, settings, WaveformSettings::kSave, WaveformSettingsLabels::SaveLabel(), nullptr, WaveformSettings::kDefaultSave);
  return page;
}
