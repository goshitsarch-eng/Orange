#include "settings/qobuzsettingspage.h"

#include "constants/qobuzsettings.h"
#include "core/application.h"
#include "settings/settingspage.h"
#include "streaming/streamingchoices.h"
#include "ui/dialogs.h"

AdwPreferencesPage *QobuzSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(QobuzSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Qobuz", "emblem-shared-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Qobuz");
  SettingsPage::AddToggle(group, settings, QobuzSettings::kEnabled, "Enable Qobuz", nullptr, QobuzSettings::kDefaultEnabled);
  SettingsPage::AddEntry(group, settings, QobuzSettings::kAppId, "App ID");
  SettingsPage::AddEntry(group, settings, QobuzSettings::kAppSecret, "App secret");
  SettingsPage::AddCombo(group, settings, QobuzSettings::kFormat, "Format", StreamingChoices::QobuzFormats(),
                         std::to_string(QobuzSettings::kDefaultFormat), [settings](const std::string &id) {
                           settings->BeginGroup(QobuzSettings::kSettingsGroup);
                           settings->SetIntValue(QobuzSettings::kFormat, static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
                           settings->Sync();
                         });
  SettingsPage::AddIntEntry(group, settings, QobuzSettings::kSearchDelay, "Search delay (ms)", QobuzSettings::kDefaultSearchDelay);
  SettingsPage::AddIntEntry(group, settings, QobuzSettings::kArtistsSearchLimit, "Artists search limit", QobuzSettings::kDefaultArtistsSearchLimit);
  SettingsPage::AddIntEntry(group, settings, QobuzSettings::kAlbumsSearchLimit, "Albums search limit", QobuzSettings::kDefaultAlbumsSearchLimit);
  SettingsPage::AddIntEntry(group, settings, QobuzSettings::kSongsSearchLimit, "Songs search limit", QobuzSettings::kDefaultSongsSearchLimit);
  SettingsPage::AddToggle(group, settings, QobuzSettings::kDownloadAlbumCovers, "Download album covers", nullptr,
                          QobuzSettings::kDefaultDownloadAlbumCovers);
  SettingsPage::AddToggle(group, settings, QobuzSettings::kRemoveRemastered, "Remove remastered from titles", nullptr,
                          QobuzSettings::kDefaultRemoveRemastered);
  if (app) {
    SettingsPage::AddButtonRow(group, "Login", "Sign in", [app]() {
      Dialogs::Login(nullptr, "Qobuz", [app](const std::string &user, const std::string &token) {
        if (StreamingService *service = app->streaming_services()->ServiceByName("Qobuz")) {
          service->Login(user, token);
        }
      });
    });
    SettingsPage::AddLoginState(group, app, "Qobuz");
  }
  return page;
}
