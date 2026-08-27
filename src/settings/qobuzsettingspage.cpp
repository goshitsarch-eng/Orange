#include "settings/qobuzsettingspage.h"

#include "constants/qobuzsettings.h"
#include "core/application.h"
#include "dialogs/messagedialog.h"
#include "qobuz/qobuzcredentialfetcher.h"
#include "settings/settingspage.h"
#include "settings/streaminglogincontrols.h"
#include "settings/streamingsettingslabels.h"
#include "streaming/streamingchoices.h"
#include "translations/translations.h"
#include "ui/dialogs.h"

AdwPreferencesPage *QobuzSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(QobuzSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Qobuz", "emblem-shared-symbolic");
  AdwPreferencesGroup *enable = SettingsPage::AddGroup(page);
  SettingsPage::AddToggle(enable, settings, QobuzSettings::kEnabled, StreamingSettingsLabels::Enable(), nullptr, QobuzSettings::kDefaultEnabled);

  AdwPreferencesGroup *auth = SettingsPage::AddGroup(page, StreamingSettingsLabels::Authentication());
  GtkWidget *app_id = SettingsPage::AddEntry(auth, settings, QobuzSettings::kAppId, StreamingSettingsLabels::AppId());
  GtkWidget *app_secret = SettingsPage::AddEntry(auth, settings, QobuzSettings::kAppSecret, QobuzSettingsLabels::AppSecret());
  GtkWidget *private_key = SettingsPage::AddEntry(auth, settings, QobuzSettings::kPrivateKey, QobuzSettingsLabels::PrivateKey());
  SettingsPage::AddButtonRow(
      auth, "", QobuzSettingsLabels::FetchCredentials(),
      [settings, app, app_id, app_secret, private_key](GtkWidget *button) {
        gtk_widget_set_sensitive(button, FALSE);
        gtk_button_set_label(GTK_BUTTON(button), Translations::CStr(QobuzSettingsLabels::Fetching()));
        QobuzCredentialFetcher::Fetch(app ? app->network() : nullptr,
                                      [settings, button, app_id, app_secret, private_key](const std::string &id, const std::string &secret,
                                                                                          const std::string &key, const std::string &error) {
                                        gtk_button_set_label(GTK_BUTTON(button), Translations::CStr(QobuzSettingsLabels::FetchCredentials()));
                                        gtk_widget_set_sensitive(button, TRUE);
                                        if (!error.empty()) {
                                          MessageDialog::Show(nullptr, QobuzSettingsLabels::CredentialFetchFailed(), error);
                                          return;
                                        }
                                        gtk_editable_set_text(GTK_EDITABLE(app_id), id.c_str());
                                        gtk_editable_set_text(GTK_EDITABLE(app_secret), secret.c_str());
                                        gtk_editable_set_text(GTK_EDITABLE(private_key), key.c_str());
                                        if (settings) {
                                          settings->BeginGroup(QobuzSettings::kSettingsGroup);
                                          settings->SetValue(QobuzSettings::kAppId, id);
                                          settings->SetValue(QobuzSettings::kAppSecret, secret);
                                          settings->SetValue(QobuzSettings::kPrivateKey, key);
                                          settings->Sync();
                                        }
                                        MessageDialog::Show(nullptr, QobuzSettingsLabels::CredentialsFetched(),
                                                            QobuzSettingsLabels::CredentialsFetchedBody());
                                      });
      },
      QobuzSettingsLabels::FetchTooltip());
  if (app) {
    GtkWidget *login_row = SettingsPage::AddButtonRow(auth, "", StreamingSettingsLabels::Login(), [app, settings](GtkWidget *button) {
      if (settings) {
        settings->BeginGroup(QobuzSettings::kSettingsGroup);
      }
      const char *missing = QobuzSettingsLabels::MissingCredentialMessage(settings ? settings->Value(QobuzSettings::kAppId) : "",
                                                                         settings ? settings->Value(QobuzSettings::kAppSecret) : "",
                                                                         settings ? settings->Value(QobuzSettings::kPrivateKey) : "");
      if (!StreamingLoginControls::ShouldDisableOnStart(missing == nullptr)) {
        MessageDialog::Show(nullptr, QobuzSettingsLabels::ConfigIncomplete(), missing);
        return;
      }
      gtk_widget_set_sensitive(button, StreamingLoginControls::LoginButtonEnabled(true));
      Dialogs::Login(nullptr, "Qobuz", [app, button](const std::string &user, const std::string &token) {
        if (StreamingService *service = app->streaming_services()->ServiceByName("Qobuz")) {
          service->Login(user, token);
        }
        gtk_widget_set_sensitive(button, StreamingLoginControls::LoginButtonEnabledAfterAuth());
      });
    });
    SettingsPage::BindLoginProgress(GTK_WIDGET(g_object_get_data(G_OBJECT(login_row), "action-button")),
                                    app->streaming_services()->ServiceByName("Qobuz"), GTK_WIDGET(page));
    SettingsPage::AddLoginState(auth, app, "Qobuz");
  }

  AdwPreferencesGroup *prefs = SettingsPage::AddGroup(page, StreamingSettingsLabels::Preferences());
  SettingsPage::AddCombo(prefs, settings, QobuzSettings::kFormat, QobuzSettingsLabels::AudioFormat(), StreamingChoices::QobuzFormats(),
                         std::to_string(QobuzSettings::kDefaultFormat), [settings](const std::string &id) {
                           settings->BeginGroup(QobuzSettings::kSettingsGroup);
                           settings->SetIntValue(QobuzSettings::kFormat, static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
                           settings->Sync();
                         });
  SettingsPage::AddIntEntry(prefs, settings, QobuzSettings::kSearchDelay, StreamingSettingsLabels::SearchDelay(), QobuzSettings::kDefaultSearchDelay);
  SettingsPage::AddIntEntry(prefs, settings, QobuzSettings::kArtistsSearchLimit, StreamingSettingsLabels::ArtistsSearchLimit(),
                            QobuzSettings::kDefaultArtistsSearchLimit);
  SettingsPage::AddIntEntry(prefs, settings, QobuzSettings::kAlbumsSearchLimit, StreamingSettingsLabels::AlbumsSearchLimit(),
                            QobuzSettings::kDefaultAlbumsSearchLimit);
  SettingsPage::AddIntEntry(prefs, settings, QobuzSettings::kSongsSearchLimit, StreamingSettingsLabels::SongsSearchLimit(),
                            QobuzSettings::kDefaultSongsSearchLimit);
  SettingsPage::AddToggle(prefs, settings, QobuzSettings::kDownloadAlbumCovers, StreamingSettingsLabels::DownloadAlbumCovers(), nullptr,
                          QobuzSettings::kDefaultDownloadAlbumCovers);
  SettingsPage::AddToggle(prefs, settings, QobuzSettings::kRemoveRemastered, StreamingSettingsLabels::RemoveRemastered(), nullptr,
                          QobuzSettings::kDefaultRemoveRemastered);
  return page;
}
