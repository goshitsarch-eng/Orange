#include "settings/moodbarsettingspage.h"

#include "constants/moodbarsettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *MoodbarSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(MoodbarSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Moodbar", "weather-clear-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Moodbar");
  SettingsPage::AddToggle(group, settings, "enabled", "Show moodbar", nullptr, true);
  SettingsPage::AddIntEntry(group, settings, MoodbarSettings::kStyle, "Style (0 normal / 1 angry / 2 frozen / 3 happy / 4 system)",
                            static_cast<int>(MoodbarSettings::kDefaultStyle));
  SettingsPage::AddToggle(group, settings, MoodbarSettings::kSave, "Save generated mood files", nullptr, MoodbarSettings::kDefaultSave);
  return page;
}
