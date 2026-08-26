#include "settings/subsonicsettingspage.h"

#include "config.h"
#include "constants/subsonicsettings.h"
#include "core/application.h"
#include "settings/settingspage.h"
#include "streaming/streamingchoices.h"
#include "ui/dialogs.h"

AdwPreferencesPage *SubsonicSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(SubsonicSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Subsonic", "network-server-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Server");
  SettingsPage::AddToggle(group, settings, SubsonicSettings::kEnabled, "Enable Subsonic", nullptr, SubsonicSettings::kDefaultEnabled);
  SettingsPage::AddEntry(group, settings, SubsonicSettings::kUrl, "Server URL");
  SettingsPage::AddEntry(group, settings, SubsonicSettings::kUsername, "Username");
  SettingsPage::AddToggle(group, settings, SubsonicSettings::kHTTP2, "HTTP/2", nullptr, SubsonicSettings::kDefaultHTTP2);
  SettingsPage::AddToggle(group, settings, SubsonicSettings::kVerifyCertificate, "Verify TLS certificate", nullptr,
                          SubsonicSettings::kDefaultVerifyCertificate);
  SettingsPage::AddToggle(group, settings, SubsonicSettings::kDownloadAlbumCovers, "Download album covers", nullptr,
                          SubsonicSettings::kDefaultDownloadAlbumCovers);
  SettingsPage::AddToggle(group, settings, SubsonicSettings::kUseAlbumIdForAlbumCovers, "Use album ID for covers", nullptr,
                          SubsonicSettings::kDefaultUseAlbumIdForAlbumCovers);
  SettingsPage::AddToggle(group, settings, SubsonicSettings::kServerSideScrobbling, "Server-side scrobbling", nullptr,
                          SubsonicSettings::kDefaultServerSideScrobbling);
  SettingsPage::AddCombo(group, settings, SubsonicSettings::kAuthMethod, "Auth method", StreamingChoices::SubsonicAuthMethods(),
                         std::to_string(static_cast<int>(SubsonicSettings::kDefaultAuthMethod)), [settings](const std::string &id) {
                           settings->BeginGroup(SubsonicSettings::kSettingsGroup);
                           settings->SetIntValue(SubsonicSettings::kAuthMethod, static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
                           settings->Sync();
                         });
  if (app) {
    SettingsPage::AddButtonRow(group, "Server login", "Sign in", [app]() {
      Dialogs::Login(nullptr, "Subsonic", [app](const std::string &user, const std::string &token) {
        if (StreamingService *service = app->streaming_services()->ServiceByName("Subsonic")) {
          service->Login(user, token);
        }
      });
    });
    SettingsPage::AddLoginState(group, app, "Subsonic");
  }
  return page;
}
