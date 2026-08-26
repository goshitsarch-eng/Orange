#include "settings/subsonicsettingspage.h"

#include "config.h"
#include "constants/subsonicsettings.h"
#include "core/application.h"
#include "dialogs/messagedialog.h"
#include "settings/settingspage.h"
#include "settings/streamingsettingslabels.h"
#include "streaming/streamingchoices.h"
#include "ui/dialogs.h"

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
  SettingsPage::AddButtonRow(auth, "", SubsonicSettingsLabels::Test(), [url, username, password]() {
    const SubsonicConnectionCheck::Result result = SubsonicConnectionCheck::Validate(
        gtk_editable_get_text(GTK_EDITABLE(url)), gtk_editable_get_text(GTK_EDITABLE(username)), gtk_editable_get_text(GTK_EDITABLE(password)));
    MessageDialog::Show(nullptr, SubsonicConnectionCheck::Title(result), SubsonicConnectionCheck::Body(result));
  });
  if (app) {
    SettingsPage::AddLoginState(auth, app, "Subsonic");
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
  return page;
}
