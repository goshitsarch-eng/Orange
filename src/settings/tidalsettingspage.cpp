#include "settings/tidalsettingspage.h"

#include "constants/tidalsettings.h"
#include "core/application.h"
#include "dialogs/messagedialog.h"
#include "settings/settingspage.h"
#include "settings/streaminglogincontrols.h"
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
    GtkWidget *login_row = SettingsPage::AddButtonRow(auth, "", StreamingSettingsLabels::Login(), [app](GtkWidget *button) {
      Settings s;
      s.BeginGroup(TidalSettings::kSettingsGroup);
      const std::string client_id = s.Value(TidalSettings::kClientId);
      if (!StreamingLoginControls::ShouldDisableOnStart(StreamingLoginControls::TidalCredentialsValid(client_id))) {
        MessageDialog::Show(nullptr, TidalSettingsLabels::ConfigIncomplete(), TidalSettingsLabels::MissingClientId());
        return;
      }
      gtk_widget_set_sensitive(button, StreamingLoginControls::LoginButtonEnabled(true));
      if (auto *service = dynamic_cast<TidalService *>(app->streaming_services()->ServiceByName("Tidal"))) {
        service->StartAuthorization(client_id);
      }
    });
    SettingsPage::BindLoginProgress(GTK_WIDGET(g_object_get_data(G_OBJECT(login_row), "action-button")),
                                    app->streaming_services()->ServiceByName("Tidal"), GTK_WIDGET(page));
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
