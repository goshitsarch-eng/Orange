#include "settings/moodbarsettingspage.h"

#include "constants/moodbarsettings.h"
#include "core/seekbarsettings.h"
#include "settings/moodbarsettingslabels.h"
#include "settings/settingspage.h"
#include "widgets/seekbarmode.h"

AdwPreferencesPage *MoodbarSettingsPage::Create(Settings *settings, Application *) {
  AdwPreferencesPage *page = SettingsPage::MakePage("Moodbar", "weather-clear-symbolic");
  settings->BeginGroup(SeekbarSettings::kSettingsGroup);
  AdwPreferencesGroup *seek = SettingsPage::AddGroup(page, "Seek bar");
  SettingsPage::AddIntCombo(seek, settings, SeekbarSettings::kSettingsGroup, SeekbarSettings::kMode, "Mode",
                            {{"0", SeekbarModeMenu::Label(SeekbarSettings::Mode::Normal)},
                             {"1", SeekbarModeMenu::Label(SeekbarSettings::Mode::Moodbar)},
                             {"2", SeekbarModeMenu::Label(SeekbarSettings::Mode::Waveform)}},
                            static_cast<int>(SeekbarSettings::kDefaultMode));
  settings->BeginGroup(MoodbarSettings::kSettingsGroup);
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Moodbar");
  SettingsPage::AddIntCombo(group, settings, MoodbarSettings::kSettingsGroup, MoodbarSettings::kStyle, MoodbarSettingsLabels::StyleLabel(),
                            MoodbarSettingsLabels::StyleChoices(), static_cast<int>(MoodbarSettings::kDefaultStyle));
  SettingsPage::AddToggle(group, settings, MoodbarSettings::kSave, MoodbarSettingsLabels::SaveLabel(), nullptr, MoodbarSettings::kDefaultSave);
  return page;
}
