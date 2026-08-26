#include "settings/subsonicsettingspage.h"

#include "config.h"
#include "constants/subsonicsettings.h"
#include "core/application.h"
#include "core/network.h"
#include "dialogs/messagedialog.h"
#include "settings/settingspage.h"
#include "settings/streamingsettingslabels.h"
#include "streaming/streamingchoices.h"
#include "subsonic/subsonicping.h"
#include "subsonic/subsonicsettingsactions.h"
#include "ui/dialogs.h"
#include "widgets/loginstatewidget.h"

AdwPreferencesPage *SubsonicSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(SubsonicSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Subsonic", "network-server-symbolic");
  AdwPreferencesGroup *enable = SettingsPage::AddGroup(page);
  SettingsPage::AddToggle(enable, settings, SubsonicSettings::kEnabled, StreamingSettingsLabels::Enable(), nullptr,
                          SubsonicSettings::kDefaultEnabled);

  AdwPreferencesGroup *server = SettingsPage::AddGroup(page, SubsonicSettingsLabels::ServerUrl());
  GtkWidget *url = SettingsPage::AddEntry(server, settings, SubsonicSettings::kUrl, SubsonicSettingsLabels::ServerUrl());

  AdwPreferencesGroup *auth = SettingsPage::AddGroup(page, StreamingSettingsLabels::Authentication());
  GtkWidget *username = SettingsPage::AddEntry(auth, settings, SubsonicSettings::kUsername, StreamingSettingsLabels::Username());
  GtkWidget *password = SettingsPage::AddPasswordEntry(auth, settings, SubsonicSettings::kPassword, StreamingSettingsLabels::Password());
  SettingsPage::AddCombo(auth, settings, SubsonicSettings::kAuthMethod, SubsonicSettingsLabels::AuthMethod(),
                         StreamingChoices::SubsonicAuthMethods(), std::to_string(static_cast<int>(SubsonicSettings::kDefaultAuthMethod)),
                         [settings](const std::string &id) {
                           settings->BeginGroup(SubsonicSettings::kSettingsGroup);
                           settings->SetIntValue(SubsonicSettings::kAuthMethod, static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
                           settings->Sync();
                         });
  SettingsPage::AddButtonRow(auth, "", SubsonicSettingsLabels::Test(), [url, username, password, settings, app](GtkWidget *button) {
    const std::string server = gtk_editable_get_text(GTK_EDITABLE(url));
    const std::string user = gtk_editable_get_text(GTK_EDITABLE(username));
    const std::string pass = gtk_editable_get_text(GTK_EDITABLE(password));
    const SubsonicConnectionCheck::Result check = SubsonicConnectionCheck::Validate(server, user, pass);
    if (check != SubsonicConnectionCheck::Result::Ok) {
      MessageDialog::Show(nullptr, SubsonicConnectionCheck::Title(check), SubsonicConnectionCheck::Body(check));
      return;
    }
    if (!app || !app->network()) {
      MessageDialog::Show(nullptr, SubsonicSettingsLabels::TestFailed(), "Network is unavailable.");
      return;
    }
    settings->BeginGroup(SubsonicSettings::kSettingsGroup);
    const bool hex_auth = settings->IntValue(SubsonicSettings::kAuthMethod, static_cast<int>(SubsonicSettings::kDefaultAuthMethod)) ==
                          static_cast<int>(SubsonicSettings::AuthMethod::Hex);
    gtk_widget_set_sensitive(button, FALSE);
    g_object_ref(button);
    app->network()->Get(SubsonicPing::Url(server, user, pass, hex_auth), [button](const NetworkAccessManager::Response &response) {
      const SubsonicPing::Result result = SubsonicPing::Parse(response.body, response.status, response.error);
      if (GTK_IS_WIDGET(button)) {
        gtk_widget_set_sensitive(button, TRUE);
      }
      g_object_unref(button);
      MessageDialog::Show(nullptr, SubsonicPing::Title(result), SubsonicPing::Body(result));
    });
  });
  if (app) {
    if (LoginStateWidget *login = SettingsPage::AddLoginState(auth, app, "Subsonic")) {
      login->AddCredentialGroup(username);
      login->AddCredentialGroup(password);
    }
  }

  AdwPreferencesGroup *prefs = SettingsPage::AddGroup(page, StreamingSettingsLabels::Preferences());
  SettingsPage::AddToggle(prefs, settings, SubsonicSettings::kHTTP2, SubsonicSettingsLabels::Http2(), nullptr, SubsonicSettings::kDefaultHTTP2);
  SettingsPage::AddToggle(prefs, settings, SubsonicSettings::kVerifyCertificate, SubsonicSettingsLabels::VerifyCertificate(), nullptr,
                          SubsonicSettings::kDefaultVerifyCertificate);
  GtkWidget *covers = SettingsPage::AddToggle(prefs, settings, SubsonicSettings::kDownloadAlbumCovers, StreamingSettingsLabels::DownloadAlbumCovers(),
                                              nullptr, SubsonicSettings::kDefaultDownloadAlbumCovers);
  GtkWidget *album_id = SettingsPage::AddToggle(prefs, settings, SubsonicSettings::kUseAlbumIdForAlbumCovers,
                                               SubsonicSettingsLabels::UseAlbumIdForCovers(), nullptr,
                                               SubsonicSettings::kDefaultUseAlbumIdForAlbumCovers);
  gtk_widget_set_sensitive(album_id, adw_switch_row_get_active(ADW_SWITCH_ROW(covers)));
  g_signal_connect(covers, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer data) {
                     gtk_widget_set_sensitive(GTK_WIDGET(data), adw_switch_row_get_active(row));
                   }),
                   album_id);
  SettingsPage::AddToggle(prefs, settings, SubsonicSettings::kServerSideScrobbling, SubsonicSettingsLabels::ServerSideScrobbling(), nullptr,
                          SubsonicSettings::kDefaultServerSideScrobbling);
  SettingsPage::AddButtonRow(prefs, "", SubsonicSettingsActions::DeleteSongs(), [app]() {
    if (app && app->collection() && app->collection()->backend()) {
      SubsonicSettingsActions::DeleteCachedSongs(app->collection()->backend());
    }
  });
  return page;
}
