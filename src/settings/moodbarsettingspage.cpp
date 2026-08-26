#include "settings/moodbarsettingspage.h"

#include "constants/moodbarsettings.h"
#include "core/seekbarsettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *MoodbarSettingsPage::Create(Settings *settings, Application *) {
  AdwPreferencesPage *page = SettingsPage::MakePage("Moodbar", "weather-clear-symbolic");
  settings->BeginGroup(SeekbarSettings::kSettingsGroup);
  AdwPreferencesGroup *seek = SettingsPage::AddGroup(page, "Seek bar");
  SettingsPage::AddIntEntry(seek, settings, SeekbarSettings::kMode, "Mode (0 normal / 1 moodbar / 2 waveform)",
                            static_cast<int>(SeekbarSettings::kDefaultMode));
  settings->BeginGroup(MoodbarSettings::kSettingsGroup);
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Moodbar");
  SettingsPage::AddIntEntry(group, settings, MoodbarSettings::kStyle, "Style (0 normal / 1 angry / 2 frozen / 3 happy / 4 system)",
                            static_cast<int>(MoodbarSettings::kDefaultStyle));
  SettingsPage::AddToggle(group, settings, MoodbarSettings::kSave, "Save generated mood files", nullptr, MoodbarSettings::kDefaultSave);
  return page;
}
