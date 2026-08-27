#include "settings/coverssettingspage.h"

#include "constants/coverssettings.h"
#include "core/application.h"
#include "covermanager/coverarttypes.h"
#include "covermanager/coverproviderauth.h"
#include "covermanager/coverproviders.h"
#include "covermanager/coverprovidersettings.h"
#include "settings/coverssettingslabels.h"
#include "settings/settingspage.h"
#include "streaming/streamingservices.h"
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
  GtkWidget *open_settings = nullptr;
  std::unique_ptr<LoginStateWidget> login;
  bool login_in_progress = false;
};

void ApplyAuthPanel(ProviderListState *state, const std::string &name, bool authentication_required, bool authenticated) {
  if (!state) {
    return;
  }
  const CoverProviderAuth::Panel panel = CoverProviderAuth::PanelFor(name, authentication_required, authenticated);
  if (state->info) {
    gtk_label_set_text(GTK_LABEL(state->info), Translations::Tr(CoverProviderAuth::SelectionStatusText(name, authentication_required, authenticated)).c_str());
  }
  if (state->authenticate) {
    gtk_widget_set_visible(state->authenticate, CoverProviderAuth::AuthenticateVisible(panel));
    gtk_widget_set_sensitive(state->authenticate, CoverProviderAuth::AuthenticateEnabled(panel, state->login_in_progress));
  }
  if (state->open_settings) {
    gtk_widget_set_visible(state->open_settings, CoverProviderAuth::OpenSettingsVisible(panel));
    if (CoverProviderAuth::OpenSettingsVisible(panel)) {
      gtk_button_set_label(GTK_BUTTON(state->open_settings), Translations::Tr(CoverProviderAuth::OpenSettingsLabel(name)).c_str());
      g_object_set_data_full(G_OBJECT(state->open_settings), "settings-page", g_strdup(CoverProviderAuth::SettingsPageName(name)), g_free);
    }
  }
  if (state->login) {
    gtk_widget_set_visible(state->login->widget(), CoverProviderAuth::LoginStateVisible(panel));
    if (CoverProviderAuth::LoginStateVisible(panel)) {
      LoginStateWidget::State login_state = LoginStateWidget::State::LoggedOut;
      if (state->login_in_progress) {
        login_state = LoginStateWidget::State::LoginInProgress;
      } else if (authenticated) {
        login_state = LoginStateWidget::State::LoggedIn;
      }
      state->login->SetLoggedIn(login_state);
    }
  }
}

bool ProviderAuthenticated(Application *app, const std::string &name) {
  if (app && app->streaming_services()) {
    if (StreamingService *service = app->streaming_services()->ServiceByName(name)) {
      return service->logged_in();
    }
  }
  return CoverProviderAuth::HasServiceToken(name);
}

void OpenCoverSettingsPage(GtkButton *btn, gpointer) {
  const char *page_name = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "settings-page"));
  GtkWidget *dialog = gtk_widget_get_ancestor(GTK_WIDGET(btn), ADW_TYPE_PREFERENCES_DIALOG);
  if (dialog && page_name) {
    adw_preferences_dialog_set_visible_page_name(ADW_PREFERENCES_DIALOG(dialog), page_name);
  }
}

void ApplySelectedProvider(ProviderListState *state) {
  if (!state || !state->list) {
    ApplyAuthPanel(state, {}, false, false);
    return;
  }
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(state->list));
  if (!row) {
    ApplyAuthPanel(state, {}, false, false);
    return;
  }
  const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "provider-name"));
  const std::string provider_name = name ? name : "";
  bool required = false;
  if (state->app && state->app->cover_providers()) {
    for (CoverProvider *provider : state->app->cover_providers()->All()) {
      if (provider && provider->name() == provider_name) {
        required = provider->authentication_required();
        break;
      }
    }
  }
  ApplyAuthPanel(state, provider_name, required, ProviderAuthenticated(state->app, provider_name));
}

struct TypeListState {
  Settings *settings = nullptr;
  GtkWidget *list = nullptr;
  std::vector<CoverArtTypes::Entry> entries;
};

struct SaveFilenameState {
  GtkWidget *filename = nullptr;
  GtkWidget *pattern = nullptr;
  GtkWidget *overwrite = nullptr;
  GtkWidget *lowercase = nullptr;
  GtkWidget *replace_spaces = nullptr;
  std::string save_type;
  std::string filename_mode;
};

void RefreshProviderList(ProviderListState *state) {
  if (!state || !state->list || !state->app) {
    return;
  }
  while (GtkWidget *child = gtk_widget_get_first_child(state->list)) {
    gtk_list_box_remove(GTK_LIST_BOX(state->list), child);
  }
  const std::vector<CoverProvider *> providers = state->app->cover_providers()->All();
  for (size_t i = 0; i < providers.size(); ++i) {
    CoverProvider *provider = providers[i];
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
                       const std::vector<CoverProvider *> providers = self->app->cover_providers()->All();
                       if (index < 0 || static_cast<size_t>(index) >= providers.size()) {
                         return;
                       }
                       self->app->cover_providers()->SetEnabled(providers[static_cast<size_t>(index)], gtk_switch_get_active(toggle));
                     }),
                     nullptr);
    GtkWidget *up = gtk_button_new_from_icon_name("go-up-symbolic");
    GtkWidget *down = gtk_button_new_from_icon_name("go-down-symbolic");
    gtk_widget_set_tooltip_text(up, Translations::CStr(CoverProviderSettings::MoveUp()));
    gtk_widget_set_tooltip_text(down, Translations::CStr(CoverProviderSettings::MoveDown()));
    gtk_widget_set_sensitive(up, CoverProviderAuth::MoveUpEnabled(static_cast<int>(i), static_cast<int>(providers.size())));
    gtk_widget_set_sensitive(down, CoverProviderAuth::MoveDownEnabled(static_cast<int>(i), static_cast<int>(providers.size())));
    g_object_set_data(G_OBJECT(up), "provider-state", state);
    g_object_set_data(G_OBJECT(down), "provider-state", state);
    g_object_set_data(G_OBJECT(up), "provider-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    g_object_set_data(G_OBJECT(down), "provider-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    g_signal_connect(up, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<ProviderListState *>(g_object_get_data(G_OBJECT(button), "provider-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "provider-index")) - 1;
                       if (self && self->app) {
                         self->app->cover_providers()->Move(index, -1);
                         RefreshProviderList(self);
                         ApplySelectedProvider(self);
                       }
                     })),
                     nullptr);
    g_signal_connect(down, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<ProviderListState *>(g_object_get_data(G_OBJECT(button), "provider-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "provider-index")) - 1;
                       if (self && self->app) {
                         self->app->cover_providers()->Move(index, 1);
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

void PersistTypes(TypeListState *state) {
  if (!state || !state->settings) {
    return;
  }
  state->settings->BeginGroup(CoversSettings::kSettingsGroup);
  state->settings->SetValue(CoversSettings::kTypes, CoverArtTypes::Save(state->entries));
  state->settings->Sync();
}

void RefreshTypeList(TypeListState *state) {
  if (!state || !state->list) {
    return;
  }
  while (GtkWidget *child = gtk_widget_get_first_child(state->list)) {
    gtk_list_box_remove(GTK_LIST_BOX(state->list), child);
  }
  for (size_t i = 0; i < state->entries.size(); ++i) {
    const CoverArtTypes::Entry &entry = state->entries[i];
    AdwSwitchRow *row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(CoverArtTypes::Description(entry.id).c_str()));
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), entry.id.c_str());
    adw_switch_row_set_active(row, entry.enabled);
    GtkWidget *up = gtk_button_new_from_icon_name("go-up-symbolic");
    GtkWidget *down = gtk_button_new_from_icon_name("go-down-symbolic");
    gtk_widget_set_sensitive(up, i > 0);
    gtk_widget_set_sensitive(down, i + 1 < state->entries.size());
    g_object_set_data(G_OBJECT(row), "type-state", state);
    g_object_set_data(G_OBJECT(up), "type-state", state);
    g_object_set_data(G_OBJECT(down), "type-state", state);
    g_object_set_data(G_OBJECT(row), "type-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    g_object_set_data(G_OBJECT(up), "type-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    g_object_set_data(G_OBJECT(down), "type-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    g_signal_connect(row, "notify::active", G_CALLBACK((+[](AdwSwitchRow *switch_row, GParamSpec *, gpointer) {
                       auto *self = static_cast<TypeListState *>(g_object_get_data(G_OBJECT(switch_row), "type-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(switch_row), "type-index")) - 1;
                       if (!self || index < 0 || index >= static_cast<int>(self->entries.size())) {
                         return;
                       }
                       self->entries[static_cast<size_t>(index)].enabled = adw_switch_row_get_active(switch_row);
                       PersistTypes(self);
                     })),
                     nullptr);
    g_signal_connect(up, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<TypeListState *>(g_object_get_data(G_OBJECT(button), "type-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "type-index")) - 1;
                       if (!self) {
                         return;
                       }
                       self->entries = CoverArtTypes::Move(self->entries, index, -1);
                       PersistTypes(self);
                       RefreshTypeList(self);
                     })),
                     nullptr);
    g_signal_connect(down, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<TypeListState *>(g_object_get_data(G_OBJECT(button), "type-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "type-index")) - 1;
                       if (!self) {
                         return;
                       }
                       self->entries = CoverArtTypes::Move(self->entries, index, 1);
                       PersistTypes(self);
                       RefreshTypeList(self);
                     })),
                     nullptr);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), up);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), down);
    gtk_list_box_append(GTK_LIST_BOX(state->list), GTK_WIDGET(row));
  }
}

void ApplySaveFilenameSensitivity(SaveFilenameState *state) {
  if (!state) {
    return;
  }
  const bool group = CoverArtTypes::FilenameGroupEnabled(state->save_type);
  const bool pattern = CoverArtTypes::FilenamePatternOptionsEnabled(state->save_type, state->filename_mode);
  if (state->filename) {
    gtk_widget_set_sensitive(state->filename, group);
  }
  if (state->pattern) {
    gtk_widget_set_sensitive(state->pattern, pattern);
  }
  if (state->overwrite) {
    gtk_widget_set_sensitive(state->overwrite, pattern);
  }
  if (state->lowercase) {
    gtk_widget_set_sensitive(state->lowercase, pattern);
  }
  if (state->replace_spaces) {
    gtk_widget_set_sensitive(state->replace_spaces, pattern);
  }
}

}  // namespace

AdwPreferencesPage *CoversSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(CoversSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Covers", "image-x-generic-symbolic");
  AdwPreferencesGroup *providers = SettingsPage::AddGroup(page, CoversSettingsLabels::ProvidersGroup());
  SettingsPage::AddDescription(providers, CoversSettingsLabels::ProvidersHint());
  if (app) {
    GtkWidget *list = gtk_list_box_new();
    gtk_widget_add_css_class(list, "boxed-list");
    auto *state = new ProviderListState();
    state->app = app;
    state->settings = settings;
    state->list = list;
    g_object_set_data_full(G_OBJECT(page), "cover-provider-state", state, [](gpointer p) { delete static_cast<ProviderListState *>(p); });
    adw_preferences_group_add(providers, list);
    RefreshProviderList(state);
    g_signal_connect(list, "row-selected", G_CALLBACK((+[](GtkListBox *, GtkListBoxRow *, gpointer data) {
                       ApplySelectedProvider(static_cast<ProviderListState *>(data));
                     })),
                     state);
  }

  AdwPreferencesGroup *types = SettingsPage::AddGroup(page, CoversSettingsLabels::TypesGroup());
  GtkWidget *types_hint = gtk_label_new(Translations::CStr("Checked types are tried in this order when loading album art."));
  gtk_label_set_wrap(GTK_LABEL(types_hint), TRUE);
  gtk_label_set_xalign(GTK_LABEL(types_hint), 0);
  gtk_widget_add_css_class(types_hint, "dim-label");
  adw_preferences_group_add(types, types_hint);
  GtkWidget *types_list = gtk_list_box_new();
  gtk_widget_add_css_class(types_list, "boxed-list");
  auto *type_state = new TypeListState();
  type_state->settings = settings;
  type_state->list = types_list;
  type_state->entries = CoverArtTypes::Parse(settings->Value(CoversSettings::kTypes, CoverArtTypes::DefaultSaved()));
  g_object_set_data_full(G_OBJECT(page), "cover-type-state", type_state, [](gpointer p) { delete static_cast<TypeListState *>(p); });
  adw_preferences_group_add(types, types_list);
  RefreshTypeList(type_state);

  AdwPreferencesGroup *save = SettingsPage::AddGroup(page, CoversSettingsLabels::SavingGroup());
  auto *filename_state = new SaveFilenameState();
  filename_state->save_type = settings->Value(CoversSettings::kSaveType, CoversSettingsLabels::DefaultSaveType());
  filename_state->filename_mode = settings->Value(CoversSettings::kSaveFilename, CoversSettingsLabels::DefaultFilename());
  g_object_set_data_full(G_OBJECT(page), "cover-filename-state", filename_state, [](gpointer p) { delete static_cast<SaveFilenameState *>(p); });
  SettingsPage::AddChoiceRadios(save, settings, CoversSettings::kSaveType, nullptr, CoversSettingsLabels::SaveTypeChoices(),
                               CoversSettingsLabels::DefaultSaveType(), [filename_state](const std::string &id) {
                                 filename_state->save_type = id;
                                 ApplySaveFilenameSensitivity(filename_state);
                               });
  filename_state->filename = SettingsPage::AddCombo(save, settings, CoversSettings::kSaveFilename, CoversSettingsLabels::FilenameGroup(),
                                                   CoversSettingsLabels::FilenameChoices(), CoversSettingsLabels::DefaultFilename(),
                                                   [filename_state](const std::string &id) {
                                                     filename_state->filename_mode = id;
                                                     ApplySaveFilenameSensitivity(filename_state);
                                                   });
  filename_state->pattern = SettingsPage::AddEntry(save, settings, CoversSettings::kSavePattern, CoversSettingsLabels::FilenamePattern(),
                                                  CoversSettings::kDefaultSavePattern);
  filename_state->overwrite = SettingsPage::AddToggle(save, settings, CoversSettings::kSaveOverwrite, CoversSettingsLabels::Overwrite(), nullptr,
                                                     CoversSettings::kDefaultSaveOverwrite);
  filename_state->lowercase = SettingsPage::AddToggle(save, settings, CoversSettings::kSaveLowercase, CoversSettingsLabels::Lowercase(), nullptr,
                                                     CoversSettings::kDefaultSaveLowercase);
  filename_state->replace_spaces = SettingsPage::AddToggle(save, settings, CoversSettings::kSaveReplaceSpaces, CoversSettingsLabels::ReplaceSpaces(),
                                                          nullptr, CoversSettings::kDefaultSaveReplaceSpaces);
  ApplySaveFilenameSensitivity(filename_state);
  SettingsPage::AddToggle(save, settings, CoversSettings::kAutomaticSearch, CoversSettingsLabels::AutomaticSearch(), nullptr,
                          CoversSettings::kDefaultAutomaticSearch);

  AdwPreferencesGroup *auth = SettingsPage::AddGroup(page, CoversSettingsLabels::Authentication());
  SettingsPage::AddDescription(auth, CoversSettingsLabels::AuthHint());
  ProviderListState *provider_state = static_cast<ProviderListState *>(g_object_get_data(G_OBJECT(page), "cover-provider-state"));
  GtkWidget *info = gtk_label_new(Translations::CStr(CoverProviderAuth::NoProviderSelected()));
  gtk_label_set_wrap(GTK_LABEL(info), TRUE);
  gtk_label_set_xalign(GTK_LABEL(info), 0.0f);
  gtk_widget_add_css_class(info, "dim-label");
  adw_preferences_group_add(auth, info);
  GtkWidget *open_settings = gtk_button_new_with_label(Translations::Tr(CoverProviderAuth::OpenSettingsLabel("Tidal")).c_str());
  gtk_widget_add_css_class(open_settings, "flat");
  gtk_widget_set_halign(open_settings, GTK_ALIGN_START);
  g_signal_connect(open_settings, "clicked", G_CALLBACK(OpenCoverSettingsPage), nullptr);
  adw_preferences_group_add(auth, open_settings);
  GtkWidget *authenticate = gtk_button_new_with_label(Translations::CStr(CoverProviderAuth::Authenticate()));
  gtk_widget_set_halign(authenticate, GTK_ALIGN_START);
  adw_preferences_group_add(auth, authenticate);
  auto login = std::make_unique<LoginStateWidget>();
  adw_preferences_group_add(auth, login->widget());
  if (provider_state) {
    provider_state->info = info;
    provider_state->open_settings = open_settings;
    provider_state->authenticate = authenticate;
    provider_state->login = std::move(login);
    g_signal_connect(authenticate, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                       auto *self = static_cast<ProviderListState *>(data);
                       if (!self) {
                         return;
                       }
                       self->login_in_progress = true;
                       ApplySelectedProvider(self);
                       GtkListBoxRow *row = self->list ? gtk_list_box_get_selected_row(GTK_LIST_BOX(self->list)) : nullptr;
                       const char *name = row ? static_cast<const char *>(g_object_get_data(G_OBJECT(row), "provider-name")) : nullptr;
                       if (name && CoverProviderAuth::ShowOpenSettings(name) && self->open_settings) {
                         gtk_button_set_label(GTK_BUTTON(self->open_settings), Translations::Tr(CoverProviderAuth::OpenSettingsLabel(name)).c_str());
                         g_object_set_data_full(G_OBJECT(self->open_settings), "settings-page",
                                                g_strdup(CoverProviderAuth::SettingsPageName(name)), g_free);
                         OpenCoverSettingsPage(GTK_BUTTON(self->open_settings), nullptr);
                       }
                       self->login_in_progress = false;
                       ApplySelectedProvider(self);
                     })),
                     provider_state);
    provider_state->login->SetLogoutCallback([provider_state]() {
      provider_state->login_in_progress = false;
      ApplySelectedProvider(provider_state);
    });
    ApplySelectedProvider(provider_state);
  } else {
    gtk_widget_set_visible(open_settings, FALSE);
    gtk_widget_set_visible(authenticate, FALSE);
    gtk_widget_set_visible(login->widget(), FALSE);
    g_object_set_data_full(G_OBJECT(page), "cover-login-widget", login.release(), [](gpointer p) { delete static_cast<LoginStateWidget *>(p); });
  }
  return page;
}
