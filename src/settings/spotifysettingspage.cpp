#include "settings/spotifysettingspage.h"

#include "constants/spotifysettings.h"
#include "core/application.h"
#include "core/oauthenticator.h"
#include "settings/settingspage.h"
#include "spotify/spotifyservice.h"
#include "ui/dialogs.h"

AdwPreferencesPage *SpotifySettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(SpotifySettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Spotify", "emblem-shared-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Spotify");
  SettingsPage::AddToggle(group, settings, SpotifySettings::kEnabled, "Enable Spotify", nullptr, SpotifySettings::kDefaultEnabled);
  SettingsPage::AddEntry(group, settings, "clientid", "Client ID");
  SettingsPage::AddEntry(group, settings, "clientsecret", "Client secret");
  SettingsPage::AddIntEntry(group, settings, SpotifySettings::kSearchDelay, "Search delay (ms)", SpotifySettings::kDefaultSearchDelay);
  SettingsPage::AddIntEntry(group, settings, SpotifySettings::kArtistsSearchLimit, "Artists search limit", SpotifySettings::kDefaultArtistsSearchLimit);
  SettingsPage::AddIntEntry(group, settings, SpotifySettings::kAlbumsSearchLimit, "Albums search limit", SpotifySettings::kDefaultAlbumsSearchLimit);
  SettingsPage::AddIntEntry(group, settings, SpotifySettings::kSongsSearchLimit, "Songs search limit", SpotifySettings::kDefaultSongsSearchLimit);
  SettingsPage::AddToggle(group, settings, SpotifySettings::kFetchAlbums, "Fetch albums when searching songs", nullptr,
                          SpotifySettings::kDefaultFetchAlbums);
  SettingsPage::AddToggle(group, settings, SpotifySettings::kDownloadAlbumCovers, "Download album covers", nullptr,
                          SpotifySettings::kDefaultDownloadAlbumCovers);
  SettingsPage::AddToggle(group, settings, SpotifySettings::kRemoveRemastered, "Remove remastered from titles", nullptr,
                          SpotifySettings::kDefaultRemoveRemastered);
  if (app) {
    SettingsPage::AddButtonRow(group, "OAuth", "Sign in", [app]() {
      Settings s;
      s.BeginGroup(SpotifySettings::kSettingsGroup);
      const std::string client_id = s.Value("clientid");
      if (client_id.empty()) {
        Dialogs::Login(nullptr, "Spotify", [app](const std::string &user, const std::string &token) {
          if (StreamingService *service = app->streaming_services()->ServiceByName("Spotify")) {
            service->Login(user, token);
          }
        });
        return;
      }
      auto *oauth = new OAuthenticator(app->network());
      oauth->AuthorizeInBrowser("https://accounts.spotify.com/authorize", client_id, "user-read-private user-read-email",
                                [app, oauth](const std::string &code, const std::string &error) {
                                  if (code.empty()) {
                                    (void)error;
                                    delete oauth;
                                    return;
                                  }
                                  Settings ss;
                                  ss.BeginGroup(SpotifySettings::kSettingsGroup);
                                  oauth->ExchangeCode("https://accounts.spotify.com/api/token", ss.Value("clientid"), ss.Value("clientsecret"), code,
                                                      [app, oauth](const std::string &body, const std::string &) {
                                                        const auto tokens = OAuthenticator::ParseTokenResponse(body);
                                                        if (auto *service = dynamic_cast<SpotifyService *>(app->streaming_services()->ServiceByName("Spotify"))) {
                                                          if (!tokens.access_token.empty()) {
                                                            service->StoreTokens(tokens);
                                                          } else {
                                                            service->Login({}, body);
                                                          }
                                                        }
                                                        delete oauth;
                                                      });
                                });
    });
    SettingsPage::AddLoginState(group, app, "Spotify");
  }
  return page;
}
