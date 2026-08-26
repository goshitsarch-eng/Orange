#include "settings/scrobblersettingspage.h"

#include "config.h"
#include "constants/scrobblersettings.h"
#include "core/application.h"
#include "settings/settingspage.h"
#include "ui/dialogs.h"

AdwPreferencesPage *ScrobblerSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(ScrobblerSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Scrobbler", "send-to-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Services");
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kEnabled, "Enable scrobbling", nullptr, ScrobblerSettings::kDefaultEnabled);
  SettingsPage::AddToggle(group, settings, "Last.fm", "Last.fm", nullptr, false);
  SettingsPage::AddToggle(group, settings, "ListenBrainz", "ListenBrainz", nullptr, false);
#ifdef HAVE_SUBSONIC
  SettingsPage::AddToggle(group, settings, "Subsonic", "Subsonic scrobble", nullptr, false);
#endif
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kScrobbleButton, "Show scrobble button", nullptr,
                          ScrobblerSettings::kDefaultScrobbleButton);
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kLoveButton, "Show love button", nullptr, ScrobblerSettings::kDefaultLoveButton);
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kOffline, "Offline mode", nullptr, ScrobblerSettings::kDefaultOffline);
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kAlbumArtist, "Prefer album artist", nullptr, ScrobblerSettings::kDefaultAlbumArtist);
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kShowErrorDialog, "Show error dialog", nullptr,
                          ScrobblerSettings::kDefaultShowErrorDialog);
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kStripRemastered, "Strip remastered from titles", nullptr,
                          ScrobblerSettings::kDefaultStripRemastered);
  SettingsPage::AddEntry(group, settings, "username", "Username");
  if (app) {
    SettingsPage::AddButtonRow(group, "Account", "Sign in", [app]() {
      Dialogs::Login(nullptr, "Last.fm", [app](const std::string &user, const std::string &pass) {
        for (ScrobblerService *service : app->scrobbler()->All()) {
          service->Authenticate(user, pass);
        }
      });
    });
  }
  return page;
}
