#include "settings/moodbarsettingspage.h"

#include "constants/moodbarsettings.h"
#include "core/seekbarsettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *MoodbarSettingsPage::Create(Settings *settings, Application *) {
  AdwPreferencesPage *page = SettingsPage::MakePage("Moodbar", "weather-clear-symbolic");
  settings->BeginGroup(SeekbarSettings::kSettingsGroup);
  AdwPreferencesGroup *seek = SettingsPage::AddGroup(page, "Seek bar");
  SettingsPage::AddIntCombo(seek, settings, SeekbarSettings::kSettingsGroup, SeekbarSettings::kMode, "Mode",
                            {{"0", "Normal"}, {"1", "Moodbar"}, {"2", "Waveform"}},
                            static_cast<int>(SeekbarSettings::kDefaultMode));
  settings->BeginGroup(MoodbarSettings::kSettingsGroup);
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Moodbar");
  SettingsPage::AddIntCombo(group, settings, MoodbarSettings::kSettingsGroup, MoodbarSettings::kStyle, "Style",
                            {{"0", "Normal"}, {"1", "Angry"}, {"2", "Frozen"}, {"3", "Happy"}, {"4", "System palette"}},
                            static_cast<int>(MoodbarSettings::kDefaultStyle));
  SettingsPage::AddToggle(group, settings, MoodbarSettings::kSave, "Save generated mood files", nullptr, MoodbarSettings::kDefaultSave);
  return page;
}
