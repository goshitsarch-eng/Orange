#include "settings/lyricssettingspage.h"

#include "constants/lyricssettings.h"
#include "core/application.h"
#include "core/oauthenticator.h"
#include "lyrics/geniuslyricsprovider.h"
#include "lyrics/lyricsproviders.h"
#include "lyrics/lyricsproviderorder.h"
#include "lyrics/lyricsprovidersettings.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "ui/dialogs.h"
#include "widgets/loginstatewidget.h"

#include <memory>

namespace {

struct ProviderListState {
  Application *app = nullptr;
  Settings *settings = nullptr;
  GtkWidget *list = nullptr;
};

void RefreshProviderList(ProviderListState *state) {
  if (!state || !state->list || !state->app) {
    return;
  }
  while (GtkWidget *child = gtk_widget_get_first_child(state->list)) {
    gtk_list_box_remove(GTK_LIST_BOX(state->list), child);
  }
  const std::vector<LyricsProvider *> providers = state->app->lyrics_providers()->All();
  for (size_t i = 0; i < providers.size(); ++i) {
    LyricsProvider *provider = providers[i];
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), provider->name().c_str());
    GtkWidget *enabled = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(enabled), provider->enabled() ? TRUE : FALSE);
    gtk_widget_set_valign(enabled, GTK_ALIGN_CENTER);
    g_object_set_data(G_OBJECT(enabled), "provider-state", state);
    g_object_set_data(G_OBJECT(enabled), "provider-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    g_signal_connect(enabled, "notify::active", G_CALLBACK(+[](GtkSwitch *toggle, GParamSpec *, gpointer) {
                       auto *self = static_cast<ProviderListState *>(g_object_get_data(G_OBJECT(toggle), "provider-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(toggle), "provider-index")) - 1;
                       if (!self || !self->app) {
                         return;
                       }
                       const std::vector<LyricsProvider *> providers = self->app->lyrics_providers()->All();
                       if (index < 0 || static_cast<size_t>(index) >= providers.size()) {
                         return;
                       }
                       self->app->lyrics_providers()->SetEnabled(providers[static_cast<size_t>(index)], gtk_switch_get_active(toggle));
                     }),
                     nullptr);
    GtkWidget *up = gtk_button_new_from_icon_name("go-up-symbolic");
    GtkWidget *down = gtk_button_new_from_icon_name("go-down-symbolic");
    gtk_widget_set_tooltip_text(up, Translations::CStr(LyricsProviderSettings::MoveUp()));
    gtk_widget_set_tooltip_text(down, Translations::CStr(LyricsProviderSettings::MoveDown()));
    gtk_widget_set_sensitive(up, i > 0);
    gtk_widget_set_sensitive(down, i + 1 < providers.size());
    g_object_set_data(G_OBJECT(up), "provider-state", state);
    g_object_set_data(G_OBJECT(down), "provider-state", state);
    g_object_set_data(G_OBJECT(up), "provider-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    g_object_set_data(G_OBJECT(down), "provider-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    g_signal_connect(up, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<ProviderListState *>(g_object_get_data(G_OBJECT(button), "provider-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "provider-index")) - 1;
                       if (self && self->app) {
                         self->app->lyrics_providers()->Move(index, -1);
                         RefreshProviderList(self);
                       }
                     })),
                     nullptr);
    g_signal_connect(down, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<ProviderListState *>(g_object_get_data(G_OBJECT(button), "provider-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "provider-index")) - 1;
                       if (self && self->app) {
                         self->app->lyrics_providers()->Move(index, 1);
                         RefreshProviderList(self);
                       }
                     })),
                     nullptr);
    adw_action_row_add_suffix(row, enabled);
    adw_action_row_add_suffix(row, up);
    adw_action_row_add_suffix(row, down);
    gtk_list_box_append(GTK_LIST_BOX(state->list), GTK_WIDGET(row));
  }
}

}  // namespace

AdwPreferencesPage *LyricsSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(LyricsSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Lyrics", "text-x-generic-symbolic");
  AdwPreferencesGroup *order = SettingsPage::AddGroup(page, LyricsProviderSettings::ProvidersGroup());
  GtkWidget *hint = gtk_label_new(Translations::CStr(LyricsProviderSettings::ProvidersHint()));
  gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
  gtk_label_set_xalign(GTK_LABEL(hint), 0);
  gtk_widget_add_css_class(hint, "dim-label");
  adw_preferences_group_add(order, hint);
  if (app) {
    GtkWidget *list = gtk_list_box_new();
    gtk_widget_add_css_class(list, "boxed-list");
    auto *state = new ProviderListState();
    state->app = app;
    state->settings = settings;
    state->list = list;
    g_object_set_data_full(G_OBJECT(page), "provider-state", state, [](gpointer p) { delete static_cast<ProviderListState *>(p); });
    adw_preferences_group_add(order, list);
    RefreshProviderList(state);
  }
  if (app) {
    if (auto *genius = dynamic_cast<GeniusLyricsProvider *>(app->lyrics_providers()->ProviderByName("Genius"))) {
      settings->BeginGroup("Genius");
      AdwPreferencesGroup *genius_group = SettingsPage::AddGroup(page, "Genius account");
      SettingsPage::AddEntry(genius_group, settings, "client_id", "Client ID");
      SettingsPage::AddEntry(genius_group, settings, "client_secret", "Client secret");
      auto *login = new LoginStateWidget();
      login->SetAccountTypeVisible(false);
      login->SetLoggedIn(genius->authenticated() ? LoginStateWidget::State::LoggedIn : LoginStateWidget::State::LoggedOut,
                         genius->username());
      auto page_alive = std::make_shared<bool>(true);
      auto *alive_ptr = new std::shared_ptr<bool>(page_alive);
      g_object_set_data_full(G_OBJECT(page), "genius-alive", alive_ptr, [](gpointer p) {
        auto *alive = static_cast<std::shared_ptr<bool> *>(p);
        **alive = false;
        delete alive;
      });
      login->SetLoginCallback([app, genius, login, page_alive, settings]() {
        settings->BeginGroup("Genius");
        const std::string client_id = settings->Value("client_id");
        if (client_id.empty()) {
          Dialogs::Login(nullptr, "Genius", [genius, login, page_alive](const std::string &user, const std::string &token) {
            genius->Authenticate(user, token);
            if (*page_alive) {
              login->SetLoggedIn(genius->authenticated() ? LoginStateWidget::State::LoggedIn : LoginStateWidget::State::LoggedOut,
                                 genius->username());
            }
          });
          return;
        }
        login->SetLoggedIn(LoginStateWidget::State::LoginInProgress);
        auto *oauth = new OAuthenticator(app->network());
        oauth->AuthorizeInBrowser(GeniusLyricsProvider::kAuthUrl, client_id, "me",
                                  [genius, login, page_alive, oauth, settings](const std::string &code, const std::string &error) {
                                    if (code.empty()) {
                                      (void)error;
                                      delete oauth;
                                      if (*page_alive) {
                                        login->SetLoggedIn(LoginStateWidget::State::LoggedOut);
                                      }
                                      return;
                                    }
                                    settings->BeginGroup("Genius");
                                    oauth->ExchangeCode(std::string(GeniusLyricsProvider::kApiUrl) + "/oauth/token",
                                                        settings->Value("client_id"), settings->Value("client_secret"), code,
                                                        [genius, login, page_alive, oauth](const std::string &body, const std::string &) {
                                                          const auto tokens = OAuthenticator::ParseTokenResponse(body);
                                                          if (!tokens.access_token.empty()) {
                                                            genius->Authenticate({}, tokens.access_token);
                                                          }
                                                          delete oauth;
                                                          if (*page_alive) {
                                                            login->SetLoggedIn(genius->authenticated() ? LoginStateWidget::State::LoggedIn
                                                                                                       : LoginStateWidget::State::LoggedOut,
                                                                               genius->username());
                                                          }
                                                        });
                                  },
                                  GeniusLyricsProvider::kOAuthPort);
      });
      login->SetLogoutCallback([genius, login, page_alive]() {
        genius->Logout();
        if (*page_alive) {
          login->SetLoggedIn(LoginStateWidget::State::LoggedOut);
        }
      });
      adw_preferences_group_add(genius_group, login->widget());
      g_signal_connect(login->widget(), "destroy", G_CALLBACK(+[](GtkWidget *, gpointer data) {
                         delete static_cast<LoginStateWidget *>(data);
                       }),
                       login);
      settings->BeginGroup(LyricsSettings::kSettingsGroup);
    }
  }
  return page;
}
