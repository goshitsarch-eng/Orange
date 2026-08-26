#include "settings/tidalsettingspage.h"

#include "constants/tidalsettings.h"
#include "core/application.h"
#include "core/oauthenticator.h"
#include "settings/settingspage.h"
#include "settings/streamingsettingslabels.h"
#include "streaming/streamingchoices.h"
#include "tidal/tidalservice.h"
#include "ui/dialogs.h"

AdwPreferencesPage *TidalSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(TidalSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Tidal", "emblem-shared-symbolic");
  AdwPreferencesGroup *enable = SettingsPage::AddGroup(page);
  SettingsPage::AddToggle(enable, settings, TidalSettings::kEnabled, StreamingSettingsLabels::Enable(), nullptr, TidalSettings::kDefaultEnabled);
  SettingsPage::AddDescription(enable, TidalSettingsLabels::Disclaimer());

  AdwPreferencesGroup *auth = SettingsPage::AddGroup(page, StreamingSettingsLabels::Authentication());
  SettingsPage::AddEntry(auth, settings, TidalSettings::kClientId, StreamingSettingsLabels::ClientId());
  SettingsPage::AddEntry(auth, settings, "clientsecret", "Client secret");
  if (app) {
    SettingsPage::AddButtonRow(auth, "", StreamingSettingsLabels::Login(), [app]() {
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
    SettingsPage::AddLoginState(auth, app, "Tidal");
  }

  AdwPreferencesGroup *prefs = SettingsPage::AddGroup(page, StreamingSettingsLabels::Preferences());
  SettingsPage::AddCombo(prefs, settings, TidalSettings::kQuality, TidalSettingsLabels::AudioQuality(), StreamingChoices::TidalQualities(),
                         TidalSettings::kDefaultQuality);
  SettingsPage::AddIntEntry(prefs, settings, TidalSettings::kSearchDelay, StreamingSettingsLabels::SearchDelay(), TidalSettings::kDefaultSearchDelay);
  SettingsPage::AddIntEntry(prefs, settings, TidalSettings::kArtistsSearchLimit, StreamingSettingsLabels::ArtistsSearchLimit(),
                            TidalSettings::kDefaultArtistsSearchLimit);
  SettingsPage::AddIntEntry(prefs, settings, TidalSettings::kAlbumsSearchLimit, StreamingSettingsLabels::AlbumsSearchLimit(),
                            TidalSettings::kDefaultAlbumsSearchLimit);
  SettingsPage::AddIntEntry(prefs, settings, TidalSettings::kSongsSearchLimit, StreamingSettingsLabels::SongsSearchLimit(),
                            TidalSettings::kDefaultSongsSearchLimit);
  SettingsPage::AddToggle(prefs, settings, TidalSettings::kDownloadAlbumCovers, StreamingSettingsLabels::DownloadAlbumCovers(), nullptr,
                          TidalSettings::kDefaultDownloadAlbumCovers);
  SettingsPage::AddToggle(prefs, settings, TidalSettings::kFetchAlbums, StreamingSettingsLabels::FetchEntireAlbums(), nullptr,
                          TidalSettings::kDefaultFetchAlbums);
  SettingsPage::AddCombo(prefs, settings, TidalSettings::kCoverSize, TidalSettingsLabels::AlbumCoverSize(), StreamingChoices::TidalCoverSizes(),
                         TidalSettings::kDefaultCoverSize);
  SettingsPage::AddCombo(prefs, settings, TidalSettings::kStreamUrl, TidalSettingsLabels::StreamUrlMethod(), StreamingChoices::TidalStreamUrlMethods(),
                         std::to_string(static_cast<int>(TidalSettings::kDefaultStreamUrl)), [settings](const std::string &id) {
                           settings->BeginGroup(TidalSettings::kSettingsGroup);
                           settings->SetIntValue(TidalSettings::kStreamUrl, static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
                           settings->Sync();
                         });
  SettingsPage::AddToggle(prefs, settings, TidalSettings::kAlbumExplicit, TidalSettingsLabels::AlbumExplicit(), nullptr,
                          TidalSettings::kDefaultAlbumExplicit);
  SettingsPage::AddToggle(prefs, settings, TidalSettings::kRemoveRemastered, StreamingSettingsLabels::RemoveRemastered(), nullptr,
                          TidalSettings::kDefaultRemoveRemastered);
  return page;
}
