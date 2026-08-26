#include "settings/coverssettingspage.h"

#include "constants/coverssettings.h"
#include "core/application.h"
#include "covermanager/coverarttypes.h"
#include "covermanager/coverproviderauth.h"
#include "covermanager/coverproviders.h"
#include "lyrics/lyricsproviderorder.h"
#include "settings/settingspage.h"
#include "streaming/streamingservices.h"
#include "translations/translations.h"

namespace {

struct ProviderListState {
  Application *app = nullptr;
  Settings *settings = nullptr;
  GtkWidget *list = nullptr;
};

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
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), provider->enabled() ? Translations::CStr("Enabled") : Translations::CStr("Disabled"));
    GtkWidget *up = gtk_button_new_from_icon_name("go-up-symbolic");
    GtkWidget *down = gtk_button_new_from_icon_name("go-down-symbolic");
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
                         self->app->cover_providers()->Move(index, -1);
                         if (self->settings) {
                           std::vector<std::string> names;
                           for (CoverProvider *item : self->app->cover_providers()->All()) {
                             names.push_back(item->name());
                           }
                           self->settings->BeginGroup(CoversSettings::kSettingsGroup);
                           self->settings->SetValue(CoversSettings::kProviders, LyricsProviderOrder::Join(names));
                           self->settings->Sync();
                         }
                         RefreshProviderList(self);
                       }
                     })),
                     nullptr);
    g_signal_connect(down, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<ProviderListState *>(g_object_get_data(G_OBJECT(button), "provider-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "provider-index")) - 1;
                       if (self && self->app) {
                         self->app->cover_providers()->Move(index, 1);
                         if (self->settings) {
                           std::vector<std::string> names;
                           for (CoverProvider *item : self->app->cover_providers()->All()) {
                             names.push_back(item->name());
                           }
                           self->settings->BeginGroup(CoversSettings::kSettingsGroup);
                           self->settings->SetValue(CoversSettings::kProviders, LyricsProviderOrder::Join(names));
                           self->settings->Sync();
                         }
                         RefreshProviderList(self);
                       }
                     })),
                     nullptr);
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
  AdwPreferencesGroup *save = SettingsPage::AddGroup(page, "Saving");
  auto *filename_state = new SaveFilenameState();
  filename_state->save_type = settings->Value(CoversSettings::kSaveType, "2");
  filename_state->filename_mode = settings->Value(CoversSettings::kSaveFilename, "2");
  g_object_set_data_full(G_OBJECT(page), "cover-filename-state", filename_state, [](gpointer p) { delete static_cast<SaveFilenameState *>(p); });

  SettingsPage::AddCombo(save, settings, CoversSettings::kSaveType, "Save destination",
                         {{"1", "Cache"}, {"2", "Album directory"}, {"3", "Embedded"}}, "2", [filename_state](const std::string &id) {
                           filename_state->save_type = id;
                           ApplySaveFilenameSensitivity(filename_state);
                         });
  filename_state->filename = SettingsPage::AddCombo(save, settings, CoversSettings::kSaveFilename, "Filename",
                                                   {{"1", "Hash"}, {"2", "Pattern"}}, "2", [filename_state](const std::string &id) {
                                                     filename_state->filename_mode = id;
                                                     ApplySaveFilenameSensitivity(filename_state);
                                                   });
  filename_state->pattern = SettingsPage::AddEntry(save, settings, CoversSettings::kSavePattern, "Cover filename pattern",
                                                  CoversSettings::kDefaultSavePattern);
  filename_state->overwrite =
      SettingsPage::AddToggle(save, settings, CoversSettings::kSaveOverwrite, "Overwrite existing covers", nullptr, CoversSettings::kDefaultSaveOverwrite);
  filename_state->lowercase =
      SettingsPage::AddToggle(save, settings, CoversSettings::kSaveLowercase, "Lowercase filenames", nullptr, CoversSettings::kDefaultSaveLowercase);
  filename_state->replace_spaces = SettingsPage::AddToggle(save, settings, CoversSettings::kSaveReplaceSpaces, "Replace spaces in filenames", nullptr,
                                                          CoversSettings::kDefaultSaveReplaceSpaces);
  ApplySaveFilenameSensitivity(filename_state);
  SettingsPage::AddToggle(save, settings, CoversSettings::kAutomaticSearch, "Search for missing covers automatically", nullptr,
                          CoversSettings::kDefaultAutomaticSearch);

  AdwPreferencesGroup *types = SettingsPage::AddGroup(page, "Cover types");
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

  AdwPreferencesGroup *enabled = SettingsPage::AddGroup(page, "Enabled providers");
  if (app) {
    for (CoverProvider *provider : app->cover_providers()->All()) {
      SettingsPage::AddToggle(enabled, settings, provider->name().c_str(), provider->name().c_str(), nullptr, provider->enabled());
    }
  }
  AdwPreferencesGroup *order = SettingsPage::AddGroup(page, "Fetch order");
  GtkWidget *hint = gtk_label_new(Translations::CStr("The first enabled provider is tried first."));
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
    g_object_set_data_full(G_OBJECT(page), "cover-provider-state", state, [](gpointer p) { delete static_cast<ProviderListState *>(p); });
    adw_preferences_group_add(order, list);
    RefreshProviderList(state);
  }

  AdwPreferencesGroup *auth = SettingsPage::AddGroup(page, "Authentication");
  GtkWidget *auth_hint = gtk_label_new(Translations::CStr("Streaming cover providers authenticate from their service settings pages."));
  gtk_label_set_wrap(GTK_LABEL(auth_hint), TRUE);
  gtk_label_set_xalign(GTK_LABEL(auth_hint), 0);
  gtk_widget_add_css_class(auth_hint, "dim-label");
  adw_preferences_group_add(auth, auth_hint);
  for (const char *name : CoverProviderAuth::ServiceSettingsProviders()) {
    bool authenticated = false;
    if (app && app->streaming_services()) {
      if (StreamingService *service = app->streaming_services()->ServiceByName(name)) {
        authenticated = service->logged_in();
      }
    }
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), name);
    const std::string status = Translations::Tr(CoverProviderAuth::StatusText(name, authenticated));
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), status.c_str());
    if (CoverProviderAuth::ShowOpenSettings(name)) {
      const std::string open_label = Translations::Tr(CoverProviderAuth::OpenSettingsLabel(name));
      GtkWidget *button = gtk_button_new_with_label(open_label.c_str());
      gtk_widget_add_css_class(button, "flat");
      g_object_set_data_full(G_OBJECT(button), "settings-page", g_strdup(CoverProviderAuth::SettingsPageName(name)), g_free);
      g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer) {
                         const char *page_name = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "settings-page"));
                         GtkWidget *dialog = gtk_widget_get_ancestor(GTK_WIDGET(btn), ADW_TYPE_PREFERENCES_DIALOG);
                         if (dialog && page_name) {
                           adw_preferences_dialog_set_visible_page_name(ADW_PREFERENCES_DIALOG(dialog), page_name);
                         }
                       }),
                       nullptr);
      adw_action_row_add_suffix(row, button);
      adw_action_row_set_activatable_widget(row, button);
    }
    adw_preferences_group_add(auth, GTK_WIDGET(row));
  }
  return page;
}
