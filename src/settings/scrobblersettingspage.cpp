#include "settings/scrobblersettingspage.h"

#include "config.h"
#include "constants/scrobblersettings.h"
#include "core/application.h"
#include "scrobbler/lastfmscrobbler.h"
#include "scrobbler/listenbrainzscrobbler.h"
#include "scrobbler/scrobblersources.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "ui/dialogs.h"
#include "widgets/loginstatewidget.h"

#include <functional>
#include <memory>

namespace {

AdwEntryRow *AddGroupedEntry(AdwPreferencesGroup *group, Settings *settings, const char *settings_group, const char *key, const char *title) {
  settings->BeginGroup(settings_group);
  AdwEntryRow *row = ADW_ENTRY_ROW(adw_entry_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  gtk_editable_set_text(GTK_EDITABLE(row), settings->Value(key).c_str());
  settings->BeginGroup(ScrobblerSettings::kSettingsGroup);
  g_object_set_data_full(G_OBJECT(row), "settings-group", g_strdup(settings_group), g_free);
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  g_signal_connect(row, "changed", G_CALLBACK(+[](AdwEntryRow *entry, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_group_name = static_cast<const char *>(g_object_get_data(G_OBJECT(entry), "settings-group"));
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(entry), "settings-key"));
                     s->BeginGroup(settings_group_name);
                     s->SetValue(settings_key, gtk_editable_get_text(GTK_EDITABLE(entry)));
                     s->Sync();
                     s->BeginGroup(ScrobblerSettings::kSettingsGroup);
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
  return row;
}

struct SourceFilterState {
  Settings *settings = nullptr;
  std::vector<AdwSwitchRow *> rows;
  std::vector<Song::Source> sources;
};

void SaveSources(SourceFilterState *state) {
  if (!state || !state->settings) {
    return;
  }
  std::vector<int> selected;
  bool all = true;
  for (size_t i = 0; i < state->rows.size(); ++i) {
    if (adw_switch_row_get_active(state->rows[i])) {
      selected.push_back(static_cast<int>(state->sources[i]));
    } else {
      all = false;
    }
  }
  state->settings->BeginGroup(ScrobblerSettings::kSettingsGroup);
  state->settings->SetValue(ScrobblerSettings::kSources, all ? std::string() : ScrobblerSources::Join(selected));
  state->settings->Sync();
}

void AttachLoginWidget(AdwPreferencesGroup *group, LoginStateWidget *login) {
  login->SetAccountTypeVisible(false);
  login->HideExpires();
  adw_preferences_group_add(group, login->widget());
  g_signal_connect(login->widget(), "destroy", G_CALLBACK(+[](GtkWidget *, gpointer data) {
                     delete static_cast<LoginStateWidget *>(data);
                   }),
                   login);
}

}  // namespace

AdwPreferencesPage *ScrobblerSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(ScrobblerSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Scrobbler", "send-to-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Services");
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kEnabled, "Enable scrobbling", nullptr, ScrobblerSettings::kDefaultEnabled);
  SettingsPage::AddToggle(group, settings, "Last.fm", "Last.fm", nullptr, false);
  SettingsPage::AddToggle(group, settings, "ListenBrainz", "ListenBrainz", nullptr, false);
#ifdef HAVE_SUBSONIC
  SettingsPage::AddToggle(group, settings, "Subsonic", "Subsonic scrobble", nullptr, false);
#endif
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kScrobbleButton, "Show scrobble button", nullptr,
                          ScrobblerSettings::kDefaultScrobbleButton);
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kLoveButton, "Show love button", nullptr, ScrobblerSettings::kDefaultLoveButton);
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kOffline, "Offline mode", nullptr, ScrobblerSettings::kDefaultOffline);
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kAlbumArtist, "Prefer album artist", nullptr, ScrobblerSettings::kDefaultAlbumArtist);
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kShowErrorDialog, "Show error dialog", nullptr,
                          ScrobblerSettings::kDefaultShowErrorDialog);
  SettingsPage::AddToggle(group, settings, ScrobblerSettings::kStripRemastered, "Strip remastered from titles", nullptr,
                          ScrobblerSettings::kDefaultStripRemastered);
  SettingsPage::AddIntEntry(group, settings, ScrobblerSettings::kSubmit, "Submit after percent played", ScrobblerSettings::kDefaultSubmit);

  AdwPreferencesGroup *sources = SettingsPage::AddGroup(page, "Sources");
  GtkWidget *hint = gtk_label_new(Translations::CStr("Leave every source enabled to scrobble everything."));
  gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
  gtk_label_set_xalign(GTK_LABEL(hint), 0);
  gtk_widget_add_css_class(hint, "dim-label");
  adw_preferences_group_add(sources, hint);
  auto *source_state = new SourceFilterState();
  source_state->settings = settings;
  source_state->sources = ScrobblerSources::All();
  const std::string saved_sources = settings->Value(ScrobblerSettings::kSources);
  g_object_set_data_full(G_OBJECT(page), "source-state", source_state, [](gpointer p) { delete static_cast<SourceFilterState *>(p); });
  for (Song::Source source : source_state->sources) {
    AdwSwitchRow *row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Song::SourceToString(source).c_str());
    adw_switch_row_set_active(row, ScrobblerSources::Allows(saved_sources, source));
    source_state->rows.push_back(row);
    g_object_set_data(G_OBJECT(row), "source-state", source_state);
    g_signal_connect(row, "notify::active", G_CALLBACK(+[](AdwSwitchRow *switch_row, GParamSpec *, gpointer) {
                       SaveSources(static_cast<SourceFilterState *>(g_object_get_data(G_OBJECT(switch_row), "source-state")));
                     }),
                     nullptr);
    adw_preferences_group_add(sources, GTK_WIDGET(row));
  }

  auto page_alive = std::make_shared<bool>(true);
  auto *alive_ptr = new std::shared_ptr<bool>(page_alive);
  g_object_set_data_full(G_OBJECT(page), "page-alive", alive_ptr, [](gpointer p) {
    auto *alive = static_cast<std::shared_ptr<bool> *>(p);
    **alive = false;
    delete alive;
  });

  if (app) {
    if (auto *lastfm = dynamic_cast<LastFmScrobbler *>(app->scrobbler()->ServiceByName("Last.fm"))) {
      AdwPreferencesGroup *lastfm_group = SettingsPage::AddGroup(page, "Last.fm account");
      auto *login = new LoginStateWidget();
      login->SetLoggedIn(lastfm->authenticated() ? LoginStateWidget::State::LoggedIn : LoginStateWidget::State::LoggedOut, lastfm->username());
      login->SetLoginCallback([lastfm, login, page_alive]() {
        login->SetLoggedIn(LoginStateWidget::State::LoginInProgress);
        lastfm->GetToken([lastfm, login, page_alive](bool ok) {
          if (!*page_alive) {
            return;
          }
          if (!ok) {
            login->SetLoggedIn(LoginStateWidget::State::LoggedOut);
            return;
          }
          lastfm->OpenAuthorizationUrl();
          login->SetLoggedIn(LoginStateWidget::State::LoggedOut, lastfm->username());
        });
      });
      login->SetLogoutCallback([lastfm, login, page_alive]() {
        lastfm->Logout();
        if (*page_alive) {
          login->SetLoggedIn(LoginStateWidget::State::LoggedOut);
        }
      });
      AttachLoginWidget(lastfm_group, login);
      SettingsPage::AddButtonRow(lastfm_group, "Browser authorization", "Complete authorization", [lastfm, login, page_alive]() {
        login->SetLoggedIn(LoginStateWidget::State::LoginInProgress);
        lastfm->CompleteAuthorization([lastfm, login, page_alive](bool ok) {
          if (!*page_alive) {
            return;
          }
          login->SetLoggedIn(ok ? LoginStateWidget::State::LoggedIn : LoginStateWidget::State::LoggedOut, lastfm->username());
        });
      });
      SettingsPage::AddButtonRow(lastfm_group, "Password", "Sign in with password", [lastfm, login, page_alive]() {
        Dialogs::Login(nullptr, "Last.fm", [lastfm, login, page_alive](const std::string &user, const std::string &pass) {
          if (!*page_alive) {
            return;
          }
          login->SetLoggedIn(LoginStateWidget::State::LoginInProgress);
          lastfm->Authenticate(user, pass, [lastfm, login, page_alive](bool ok) {
            if (!*page_alive) {
              return;
            }
            login->SetLoggedIn(ok ? LoginStateWidget::State::LoggedIn : LoginStateWidget::State::LoggedOut, lastfm->username());
          });
        });
      });
    }

    if (auto *listenbrainz = dynamic_cast<ListenBrainzScrobbler *>(app->scrobbler()->ServiceByName("ListenBrainz"))) {
      AdwPreferencesGroup *lb_group = SettingsPage::AddGroup(page, "ListenBrainz account");
      AdwEntryRow *user_row = AddGroupedEntry(lb_group, settings, "ListenBrainz", "username", "Username");
      AdwEntryRow *token_row = AddGroupedEntry(lb_group, settings, "ListenBrainz", "token", "User token");
      auto *login = new LoginStateWidget();
      login->SetLoggedIn(listenbrainz->authenticated() ? LoginStateWidget::State::LoggedIn : LoginStateWidget::State::LoggedOut,
                         listenbrainz->username());
      auto *apply_token = new std::function<void()>([listenbrainz, login, user_row, token_row, page_alive]() {
        listenbrainz->Authenticate(gtk_editable_get_text(GTK_EDITABLE(user_row)), gtk_editable_get_text(GTK_EDITABLE(token_row)));
        if (*page_alive) {
          login->SetLoggedIn(listenbrainz->authenticated() ? LoginStateWidget::State::LoggedIn : LoginStateWidget::State::LoggedOut,
                             listenbrainz->username());
        }
      });
      g_object_set_data_full(G_OBJECT(page), "listenbrainz-apply", apply_token, [](gpointer p) { delete static_cast<std::function<void()> *>(p); });
      login->SetLoginCallback([apply_token]() { (*apply_token)(); });
      login->SetLogoutCallback([listenbrainz, login, user_row, token_row, page_alive]() {
        listenbrainz->Logout();
        if (*page_alive) {
          gtk_editable_set_text(GTK_EDITABLE(user_row), "");
          gtk_editable_set_text(GTK_EDITABLE(token_row), "");
          login->SetLoggedIn(LoginStateWidget::State::LoggedOut);
        }
      });
      login->AddCredentialGroup(GTK_WIDGET(user_row));
      login->AddCredentialGroup(GTK_WIDGET(token_row));
      g_signal_connect(user_row, "changed", G_CALLBACK(+[](AdwEntryRow *, gpointer data) {
                         (*static_cast<std::function<void()> *>(data))();
                       }),
                       apply_token);
      g_signal_connect(token_row, "changed", G_CALLBACK(+[](AdwEntryRow *, gpointer data) {
                         (*static_cast<std::function<void()> *>(data))();
                       }),
                       apply_token);
      AttachLoginWidget(lb_group, login);
    }
  }

  return page;
}
