#include "settings/lyricssettingspage.h"

#include "constants/lyricssettings.h"
#include "core/application.h"
#include "lyrics/geniuslyricscredentials.h"
#include "lyrics/geniuslyricsprovider.h"
#include "lyrics/lyricsproviderauth.h"
#include "lyrics/lyricsproviders.h"
#include "lyrics/lyricsproviderorder.h"
#include "lyrics/lyricsprovidersettings.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "widgets/loginstatewidget.h"

#include <memory>

namespace {

struct ProviderListState {
  Application *app = nullptr;
  Settings *settings = nullptr;
  GtkWidget *list = nullptr;
  GtkWidget *info = nullptr;
  GtkWidget *authenticate = nullptr;
  GtkWidget *client_id = nullptr;
  GtkWidget *client_secret = nullptr;
  std::unique_ptr<LoginStateWidget> login;
  std::shared_ptr<bool> page_alive;
  bool login_in_progress = false;
};

void ApplyAuthPanel(ProviderListState *state, const std::string &name, bool authentication_required, bool authenticated,
                    const std::string &username) {
  if (!state) {
    return;
  }
  const LyricsProviderAuth::Panel panel = LyricsProviderAuth::PanelFor(name, authentication_required);
  if (state->info) {
    gtk_label_set_text(GTK_LABEL(state->info),
                       Translations::Tr(LyricsProviderAuth::SelectionStatusText(name, authentication_required)).c_str());
  }
  if (state->authenticate) {
    gtk_widget_set_visible(state->authenticate, LyricsProviderAuth::AuthenticateVisible(panel));
    gtk_widget_set_sensitive(state->authenticate, LyricsProviderAuth::AuthenticateEnabled(panel, state->login_in_progress));
  }
  if (state->client_id) {
    gtk_widget_set_visible(state->client_id, LyricsProviderAuth::CredentialsVisible(panel, GeniusLyricsCredentials::kHideManualFields));
  }
  if (state->client_secret) {
    gtk_widget_set_visible(state->client_secret,
                          LyricsProviderAuth::CredentialsVisible(panel, GeniusLyricsCredentials::kHideManualFields));
  }
  if (state->login) {
    gtk_widget_set_visible(state->login->widget(), LyricsProviderAuth::LoginStateVisible(panel));
    if (LyricsProviderAuth::LoginStateVisible(panel)) {
      LoginStateWidget::State login_state = LoginStateWidget::State::LoggedOut;
      if (state->login_in_progress) {
        login_state = LoginStateWidget::State::LoginInProgress;
      } else if (authenticated) {
        login_state = LoginStateWidget::State::LoggedIn;
      }
      state->login->SetLoggedIn(login_state, username);
    }
  }
}

void ApplySelectedProvider(ProviderListState *state) {
  if (!state || !state->list) {
    ApplyAuthPanel(state, {}, false, false, {});
    return;
  }
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(state->list));
  if (!row) {
    ApplyAuthPanel(state, {}, false, false, {});
    return;
  }
  const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "provider-name"));
  const std::string provider_name = name ? name : "";
  bool required = LyricsProviderAuth::RequiresAuthentication(provider_name);
  bool authenticated = false;
  std::string username;
  if (state->app && state->app->lyrics_providers()) {
    if (LyricsProvider *provider = state->app->lyrics_providers()->ProviderByName(provider_name)) {
      required = provider->authentication_required();
      authenticated = provider->authenticated();
      username = provider->username();
    }
  }
  ApplyAuthPanel(state, provider_name, required, authenticated, username);
}

void FinishGeniusLogin(ProviderListState *state) {
  if (!state) {
    return;
  }
  state->login_in_progress = false;
  ApplySelectedProvider(state);
}

void StartGeniusLogin(ProviderListState *state) {
  if (!state || !state->app || !state->settings || !state->page_alive || !*state->page_alive) {
    return;
  }
  auto *genius = dynamic_cast<GeniusLyricsProvider *>(state->app->lyrics_providers()->ProviderByName("Genius"));
  if (!genius) {
    return;
  }
  state->login_in_progress = true;
  ApplySelectedProvider(state);
  auto page_alive = state->page_alive;
  genius->Authenticate(state->app->network(), [state, page_alive]() {
    if (*page_alive) {
      FinishGeniusLogin(state);
    }
  });
}

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
    g_object_set_data_full(G_OBJECT(row), "provider-name", g_strdup(provider->name().c_str()), g_free);
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
    gtk_widget_set_sensitive(up, LyricsProviderAuth::MoveUpEnabled(static_cast<int>(i), static_cast<int>(providers.size())));
    gtk_widget_set_sensitive(down, LyricsProviderAuth::MoveDownEnabled(static_cast<int>(i), static_cast<int>(providers.size())));
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
                         ApplySelectedProvider(self);
                       }
                     })),
                     nullptr);
    g_signal_connect(down, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<ProviderListState *>(g_object_get_data(G_OBJECT(button), "provider-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "provider-index")) - 1;
                       if (self && self->app) {
                         self->app->lyrics_providers()->Move(index, 1);
                         RefreshProviderList(self);
                         ApplySelectedProvider(self);
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

  auto *state = new ProviderListState();
  state->app = app;
  state->settings = settings;
  state->page_alive = std::make_shared<bool>(true);
  g_object_set_data_full(G_OBJECT(page), "provider-state", state, [](gpointer p) {
    auto *self = static_cast<ProviderListState *>(p);
    if (self->page_alive) {
      *self->page_alive = false;
    }
    delete self;
  });

  if (app) {
    GtkWidget *list = gtk_list_box_new();
    gtk_widget_add_css_class(list, "boxed-list");
    state->list = list;
    adw_preferences_group_add(order, list);
    RefreshProviderList(state);
    g_signal_connect(list, "row-selected", G_CALLBACK((+[](GtkListBox *, GtkListBoxRow *, gpointer data) {
                       ApplySelectedProvider(static_cast<ProviderListState *>(data));
                     })),
                     state);
  }

  AdwPreferencesGroup *auth = SettingsPage::AddGroup(page, LyricsProviderAuth::AuthenticationGroup());
  state->info = gtk_label_new(Translations::CStr(LyricsProviderAuth::NoProviderSelected()));
  gtk_label_set_wrap(GTK_LABEL(state->info), TRUE);
  gtk_label_set_xalign(GTK_LABEL(state->info), 0.0f);
  gtk_widget_add_css_class(state->info, "dim-label");
  adw_preferences_group_add(auth, state->info);

  settings->BeginGroup("Genius");
  state->client_id = SettingsPage::AddEntry(auth, settings, "client_id", "Client ID");
  state->client_secret = SettingsPage::AddEntry(auth, settings, "client_secret", "Client secret");
  settings->BeginGroup(LyricsSettings::kSettingsGroup);

  state->authenticate = gtk_button_new_with_label(Translations::CStr(LyricsProviderAuth::Login()));
  gtk_widget_set_halign(state->authenticate, GTK_ALIGN_START);
  adw_preferences_group_add(auth, state->authenticate);
  state->login = std::make_unique<LoginStateWidget>();
  state->login->SetAccountTypeVisible(false);
  adw_preferences_group_add(auth, state->login->widget());

  g_signal_connect(state->authenticate, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     StartGeniusLogin(static_cast<ProviderListState *>(data));
                   })),
                   state);
  state->login->SetLoginCallback([state]() { StartGeniusLogin(state); });
  state->login->SetLogoutCallback([state]() {
    if (!state || !state->app || !state->page_alive || !*state->page_alive) {
      return;
    }
    if (LyricsProvider *provider = state->app->lyrics_providers()->ProviderByName("Genius")) {
      provider->Logout();
    }
    state->login_in_progress = false;
    ApplySelectedProvider(state);
  });
  ApplySelectedProvider(state);
  return page;
}
