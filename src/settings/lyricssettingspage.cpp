#include "settings/lyricssettingspage.h"

#include "constants/lyricssettings.h"
#include "core/application.h"
#include "settings/settingspage.h"

AdwPreferencesPage *LyricsSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(LyricsSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Lyrics", "text-x-generic-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Providers");
  if (app) {
    for (LyricsProvider *provider : app->lyrics_providers()->All()) {
      SettingsPage::AddToggle(group, settings, provider->name().c_str(), provider->name().c_str(), nullptr, true);
    }
  }
  return page;
}
