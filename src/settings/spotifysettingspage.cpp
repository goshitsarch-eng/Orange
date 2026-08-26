#include "settings/spotifysettingspage.h"

#include "constants/spotifysettings.h"
#include "core/application.h"
#include "core/oauthenticator.h"
#include "settings/settingspage.h"
#include "settings/streamingsettingslabels.h"
#include "spotify/spotifyservice.h"
#include "ui/dialogs.h"

#include <gst/gst.h>

AdwPreferencesPage *SpotifySettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(SpotifySettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Spotify", "emblem-shared-symbolic");
  AdwPreferencesGroup *enable = SettingsPage::AddGroup(page);
  SettingsPage::AddToggle(enable, settings, SpotifySettings::kEnabled, StreamingSettingsLabels::Enable(), nullptr,
                          SpotifySettings::kDefaultEnabled);

  AdwPreferencesGroup *auth = SettingsPage::AddGroup(page, SpotifySettingsLabels::BasicAuth());
  if (app) {
    SettingsPage::AddButtonRow(auth, "", SpotifySettingsLabels::Authenticate(), [app]() {
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
    SettingsPage::AddLoginState(auth, app, "Spotify");
  }

  bool has_plugin = false;
  if (GstRegistry *reg = gst_registry_get()) {
    if (GstPluginFeature *feature = gst_registry_lookup_feature(reg, SpotifySettingsLabels::PluginFeature())) {
      gst_object_unref(feature);
      has_plugin = true;
    }
  }
  if (!has_plugin) {
    AdwPreferencesGroup *warning = SettingsPage::AddGroup(page);
    const std::string markup = SpotifySettingsLabels::PluginWarningMarkup();
    SettingsPage::AddDescription(warning, markup.c_str(), true);
  }

  AdwPreferencesGroup *prefs = SettingsPage::AddGroup(page, StreamingSettingsLabels::Preferences());
  SettingsPage::AddIntEntry(prefs, settings, SpotifySettings::kSearchDelay, StreamingSettingsLabels::SearchDelay(),
                            SpotifySettings::kDefaultSearchDelay);
  SettingsPage::AddIntEntry(prefs, settings, SpotifySettings::kArtistsSearchLimit, StreamingSettingsLabels::ArtistsSearchLimit(),
                            SpotifySettings::kDefaultArtistsSearchLimit);
  SettingsPage::AddIntEntry(prefs, settings, SpotifySettings::kAlbumsSearchLimit, StreamingSettingsLabels::AlbumsSearchLimit(),
                            SpotifySettings::kDefaultAlbumsSearchLimit);
  SettingsPage::AddIntEntry(prefs, settings, SpotifySettings::kSongsSearchLimit, StreamingSettingsLabels::SongsSearchLimit(),
                            SpotifySettings::kDefaultSongsSearchLimit);
  SettingsPage::AddToggle(prefs, settings, SpotifySettings::kDownloadAlbumCovers, StreamingSettingsLabels::DownloadAlbumCovers(), nullptr,
                          SpotifySettings::kDefaultDownloadAlbumCovers);
  SettingsPage::AddToggle(prefs, settings, SpotifySettings::kFetchAlbums, StreamingSettingsLabels::FetchEntireAlbums(), nullptr,
                          SpotifySettings::kDefaultFetchAlbums);
  SettingsPage::AddToggle(prefs, settings, SpotifySettings::kRemoveRemastered, StreamingSettingsLabels::RemoveRemastered(), nullptr,
                          SpotifySettings::kDefaultRemoveRemastered);
  return page;
}
