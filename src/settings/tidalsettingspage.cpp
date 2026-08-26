#include "settings/tidalsettingspage.h"

#include "constants/tidalsettings.h"
#include "core/application.h"
#include "core/oauthenticator.h"
#include "settings/settingspage.h"
#include "tidal/tidalservice.h"
#include "ui/dialogs.h"

AdwPreferencesPage *TidalSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(TidalSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Tidal", "emblem-shared-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Tidal");
  SettingsPage::AddToggle(group, settings, TidalSettings::kEnabled, "Enable Tidal", nullptr, TidalSettings::kDefaultEnabled);
  SettingsPage::AddEntry(group, settings, TidalSettings::kClientId, "Client ID");
  SettingsPage::AddEntry(group, settings, "clientsecret", "Client secret");
  SettingsPage::AddEntry(group, settings, TidalSettings::kQuality, "Quality", TidalSettings::kDefaultQuality);
  SettingsPage::AddIntEntry(group, settings, TidalSettings::kStreamUrl, "Stream URL method",
                            static_cast<int>(TidalSettings::kDefaultStreamUrl));
  SettingsPage::AddIntEntry(group, settings, TidalSettings::kSearchDelay, "Search delay (ms)", TidalSettings::kDefaultSearchDelay);
  SettingsPage::AddIntEntry(group, settings, TidalSettings::kArtistsSearchLimit, "Artists search limit", TidalSettings::kDefaultArtistsSearchLimit);
  SettingsPage::AddIntEntry(group, settings, TidalSettings::kAlbumsSearchLimit, "Albums search limit", TidalSettings::kDefaultAlbumsSearchLimit);
  SettingsPage::AddIntEntry(group, settings, TidalSettings::kSongsSearchLimit, "Songs search limit", TidalSettings::kDefaultSongsSearchLimit);
  SettingsPage::AddToggle(group, settings, TidalSettings::kFetchAlbums, "Fetch albums when searching songs", nullptr, TidalSettings::kDefaultFetchAlbums);
  SettingsPage::AddToggle(group, settings, TidalSettings::kDownloadAlbumCovers, "Download album covers", nullptr,
                          TidalSettings::kDefaultDownloadAlbumCovers);
  SettingsPage::AddEntry(group, settings, TidalSettings::kCoverSize, "Cover size", TidalSettings::kDefaultCoverSize);
  SettingsPage::AddToggle(group, settings, TidalSettings::kRemoveRemastered, "Remove remastered from titles", nullptr,
                          TidalSettings::kDefaultRemoveRemastered);
  SettingsPage::AddEntry(group, settings, "countrycode", "Country code", "US");
  if (app) {
    SettingsPage::AddButtonRow(group, "OAuth", "Sign in", [app]() {
      Settings s;
      s.BeginGroup(TidalSettings::kSettingsGroup);
      const std::string client_id = s.Value(TidalSettings::kClientId);
      if (client_id.empty()) {
        Dialogs::Login(nullptr, "Tidal", [app](const std::string &user, const std::string &token) {
          if (StreamingService *service = app->streaming_services()->ServiceByName("Tidal")) {
            service->Login(user, token);
          }
        });
        return;
      }
      auto *oauth = new OAuthenticator(app->network());
      oauth->AuthorizeInBrowser("https://login.tidal.com/authorize", client_id, "r_usr r_res w_usr",
                                [app, oauth](const std::string &code, const std::string &error) {
                                  if (!code.empty()) {
                                    Settings ts;
                                    ts.BeginGroup(TidalSettings::kSettingsGroup);
                                    oauth->ExchangeCode("https://auth.tidal.com/v1/oauth2/token", ts.Value(TidalSettings::kClientId),
                                                        ts.Value("clientsecret"), code,
                                                        [app, oauth](const std::string &body, const std::string &) {
                                                          const auto tokens = OAuthenticator::ParseTokenResponse(body);
                                                          if (auto *service = dynamic_cast<TidalService *>(app->streaming_services()->ServiceByName("Tidal"))) {
                                                            if (!tokens.access_token.empty()) {
                                                              service->StoreTokens(tokens);
                                                            } else {
                                                              service->Login({}, body);
                                                            }
                                                          }
                                                          delete oauth;
                                                        });
                                  } else {
                                    (void)error;
                                    delete oauth;
                                  }
                                });
    });
    SettingsPage::AddLoginState(group, app, "Tidal");
  }
  return page;
}
