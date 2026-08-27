#include "config.h"
#include "settings/globalshortcutssettingspage.h"

#include "constants/globalshortcutssettings.h"
#include "core/application.h"
#include "core/settings.h"
#include "globalshortcuts/globalshortcutbinding.h"
#include "globalshortcuts/globalshortcuts.h"
#include "globalshortcuts/globalshortcutsbackend-kglobalaccel.h"
#include "globalshortcuts/macosaccessibility.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "ui/dialogs.h"

namespace {

struct BackendEnableState {
  GtkWidget *kglobalaccel = nullptr;
  GtkWidget *x11 = nullptr;
  GtkWidget *keys = nullptr;
  GtkWidget *warning = nullptr;
  bool kglobalaccel_visible = false;
  bool x11_visible = false;
};

void ApplyShortcutListEnable(BackendEnableState *state) {
  if (!state) {
    return;
  }
  const bool k_on = state->kglobalaccel && adw_switch_row_get_active(ADW_SWITCH_ROW(state->kglobalaccel));
  const bool x_on = state->x11 && adw_switch_row_get_active(ADW_SWITCH_ROW(state->x11));
  if (state->keys) {
    gtk_widget_set_sensitive(state->keys, GlobalShortcutBinding::ShortcutListEnabled(state->kglobalaccel_visible, k_on, state->x11_visible, x_on));
  }
  if (state->warning) {
    gtk_widget_set_visible(state->warning, GlobalShortcutBinding::X11WarningVisible(state->x11_visible, x_on));
  }
}

struct MacAccessMap {
  GtkWidget *group = nullptr;
  Application *app = nullptr;
};

struct ShortcutRowWidgets {
  Settings *settings = nullptr;
  Application *app = nullptr;
  GtkWidget *combo = nullptr;
  GtkWidget *entry = nullptr;
  GtkWidget *grab = nullptr;
  char *id = nullptr;
  char *default_key = nullptr;
  char *description = nullptr;
};

void FreeShortcutRowWidgets(void *data) {
  auto *row = static_cast<ShortcutRowWidgets *>(data);
  g_free(row->id);
  g_free(row->default_key);
  g_free(row->description);
  delete row;
}

void PersistKey(ShortcutRowWidgets *row, const std::string &key) {
  if (!row || !row->settings || !row->id) {
    return;
  }
  row->settings->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
  row->settings->SetValue(row->id, key);
  row->settings->Sync();
  if (row->app && row->app->shortcuts()) {
    row->app->shortcuts()->ReloadSettings();
  }
}

void ClearDuplicateKeys(Settings *settings, const char *keep_id, const std::string &key) {
  if (!settings || !keep_id || key.empty()) {
    return;
  }
  settings->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
  for (const auto &def : GlobalShortcutsManager::Catalog()) {
    if (def.id == std::string(keep_id)) {
      continue;
    }
    const std::string other = GlobalShortcutBinding::ResolveStoredKey(settings->Contains(def.id), settings->Value(def.id), false, {},
                                                                      def.default_key ? def.default_key : "");
    if (other == key) {
      settings->SetValue(def.id, {});
    }
  }
}

void SyncFromMode(ShortcutRowWidgets *row) {
  if (!row || !row->combo || !row->entry || !row->grab) {
    return;
  }
  const GlobalShortcutBinding::Mode mode =
      GlobalShortcutBinding::FromIndex(static_cast<int>(adw_combo_row_get_selected(ADW_COMBO_ROW(row->combo))));
  const bool custom = GlobalShortcutBinding::CustomEnabled(mode);
  gtk_widget_set_sensitive(row->entry, custom ? TRUE : FALSE);
  gtk_widget_set_sensitive(row->grab, custom ? TRUE : FALSE);
  const std::string typed = gtk_editable_get_text(GTK_EDITABLE(row->entry));
  const std::string key = GlobalShortcutBinding::EffectiveKey(mode, typed, row->default_key ? row->default_key : "");
  if (!custom) {
    gtk_editable_set_text(GTK_EDITABLE(row->entry), key.c_str());
  }
  PersistKey(row, key);
}

}  // namespace

AdwPreferencesPage *GlobalShortcutsSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage(GlobalShortcutBinding::PageTitle(), "input-keyboard-symbolic");
  AdwPreferencesGroup *backends = SettingsPage::AddGroup(page, "Backends");
  SettingsPage::AddToggle(backends, settings, "enabled", "Enable global shortcuts", nullptr, true);
  auto *enable = new BackendEnableState();
  enable->kglobalaccel_visible = GlobalShortcutsBackendKGlobalAccel::IsKGlobalAccelAvailable();
#ifdef HAVE_X11
  enable->x11_visible = true;
#endif
  enable->kglobalaccel = SettingsPage::AddToggle(backends, settings, GlobalShortcutsSettings::kUseKGlobalAccel,
                                                GlobalShortcutBinding::UseKGlobalAccel(), nullptr,
                                                GlobalShortcutsSettings::kDefaultUseKGlobalAccel);
  gtk_widget_set_visible(enable->kglobalaccel, enable->kglobalaccel_visible);
  enable->x11 = SettingsPage::AddToggle(backends, settings, GlobalShortcutsSettings::kUseX11, GlobalShortcutBinding::UseX11(), nullptr,
                                       GlobalShortcutsSettings::kDefaultUseX11);
  gtk_widget_set_visible(enable->x11, enable->x11_visible);
  enable->warning = gtk_label_new(GlobalShortcutBinding::X11Warning());
  gtk_label_set_wrap(GTK_LABEL(enable->warning), TRUE);
  gtk_label_set_xalign(GTK_LABEL(enable->warning), 0.0f);
  gtk_widget_set_margin_start(enable->warning, 12);
  gtk_widget_set_margin_end(enable->warning, 12);
  adw_preferences_group_add(backends, enable->warning);
  g_object_set_data_full(G_OBJECT(page), "shortcut-enable", enable, [](gpointer p) { delete static_cast<BackendEnableState *>(p); });
  g_signal_connect(enable->kglobalaccel, "notify::active", G_CALLBACK((+[](AdwSwitchRow *, GParamSpec *, gpointer data) {
                     ApplyShortcutListEnable(static_cast<BackendEnableState *>(data));
                   })),
                   enable);
  g_signal_connect(enable->x11, "notify::active", G_CALLBACK((+[](AdwSwitchRow *, GParamSpec *, gpointer data) {
                     ApplyShortcutListEnable(static_cast<BackendEnableState *>(data));
                   })),
                   enable);

  AdwPreferencesGroup *keys = SettingsPage::AddGroup(page, "Shortcuts");
  enable->keys = GTK_WIDGET(keys);
  for (const auto &def : GlobalShortcutsManager::Catalog()) {
    settings->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
    const bool contains = settings->Contains(def.id);
    const std::string stored = settings->Value(def.id);
    const GlobalShortcutBinding::Mode mode = GlobalShortcutBinding::FromSettings(contains, stored, def.default_key);
    const std::string display = GlobalShortcutBinding::EffectiveKey(mode, stored, def.default_key);

    AdwComboRow *combo = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(combo),
                                 Translations::CStr(GlobalShortcutBinding::ShortcutFor(def.description).c_str()));
    GtkStringList *model = gtk_string_list_new(nullptr);
    gtk_string_list_append(model, Translations::CStr(GlobalShortcutBinding::NoneLabel()));
    gtk_string_list_append(model, Translations::CStr(GlobalShortcutBinding::DefaultLabel()));
    gtk_string_list_append(model, Translations::CStr(GlobalShortcutBinding::CustomLabel()));
    adw_combo_row_set_model(combo, G_LIST_MODEL(model));
    adw_combo_row_set_selected(combo, static_cast<guint>(GlobalShortcutBinding::IndexOf(mode)));

    GtkWidget *entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(entry), display.c_str());
    gtk_widget_set_size_request(entry, 140, -1);
    gtk_widget_set_sensitive(entry, GlobalShortcutBinding::CustomEnabled(mode) ? TRUE : FALSE);

    GtkWidget *grab = gtk_button_new_with_label(Translations::CStr(GlobalShortcutBinding::ChangeShortcut()));
    gtk_widget_set_sensitive(grab, GlobalShortcutBinding::CustomEnabled(mode) ? TRUE : FALSE);

    auto *row = new ShortcutRowWidgets();
    row->settings = settings;
    row->app = app;
    row->combo = GTK_WIDGET(combo);
    row->entry = entry;
    row->grab = grab;
    row->id = g_strdup(def.id);
    row->default_key = g_strdup(def.default_key);
    row->description = g_strdup(def.description);
    g_object_set_data_full(G_OBJECT(combo), "shortcut-row", row, FreeShortcutRowWidgets);
    g_object_set_data(G_OBJECT(entry), "shortcut-row", row);
    g_object_set_data(G_OBJECT(grab), "shortcut-row", row);

    g_signal_connect(combo, "notify::selected", G_CALLBACK(+[](AdwComboRow *widget, GParamSpec *, gpointer) {
                       SyncFromMode(static_cast<ShortcutRowWidgets *>(g_object_get_data(G_OBJECT(widget), "shortcut-row")));
                     }),
                     nullptr);
    g_signal_connect(entry, "changed", G_CALLBACK(+[](GtkEditable *editable, gpointer) {
                       auto *widgets = static_cast<ShortcutRowWidgets *>(g_object_get_data(G_OBJECT(editable), "shortcut-row"));
                       if (!widgets || !widgets->combo) {
                         return;
                       }
                       const GlobalShortcutBinding::Mode current =
                           GlobalShortcutBinding::FromIndex(static_cast<int>(adw_combo_row_get_selected(ADW_COMBO_ROW(widgets->combo))));
                       if (!GlobalShortcutBinding::CustomEnabled(current)) {
                         return;
                       }
                       PersistKey(widgets, gtk_editable_get_text(editable));
                     }),
                     nullptr);
    g_signal_connect(grab, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                       auto *widgets = static_cast<ShortcutRowWidgets *>(g_object_get_data(G_OBJECT(button), "shortcut-row"));
                       if (!widgets) {
                         return;
                       }
                       Dialogs::GrabShortcut(
                           nullptr,
                           [widgets](const std::string &accel) {
                             if (accel.empty() || !widgets->entry) {
                               return;
                             }
                             ClearDuplicateKeys(widgets->settings, widgets->id, accel);
                             gtk_editable_set_text(GTK_EDITABLE(widgets->entry), accel.c_str());
                             PersistKey(widgets, accel);
                           },
                           widgets->description ? widgets->description : "");
                     }),
                     nullptr);

    adw_action_row_add_suffix(ADW_ACTION_ROW(combo), entry);
    adw_action_row_add_suffix(ADW_ACTION_ROW(combo), grab);
    adw_preferences_group_add(keys, GTK_WIDGET(combo));
  }
  ApplyShortcutListEnable(enable);

  AdwPreferencesGroup *access = SettingsPage::AddGroup(page, MacOsAccessibility::GroupTitle());
  adw_preferences_group_set_description(access, Translations::CStr(MacOsAccessibility::Warning()));
  SettingsPage::AddButtonRow(access, "", MacOsAccessibility::OpenButton(), [app]() {
    if (app && app->shortcuts()) {
      app->shortcuts()->ShowMacAccessibilityDialog();
    }
  });
  const bool access_enabled = app && app->shortcuts() ? app->shortcuts()->IsMacAccessibilityEnabled() : false;
  gtk_widget_set_visible(GTK_WIDGET(access), MacOsAccessibility::ShouldShowAccessRow(MacOsAccessibility::IsMacOs(), access_enabled));
  auto *access_map = new MacAccessMap{GTK_WIDGET(access), app};
  g_object_set_data_full(G_OBJECT(page), "macos-access", access_map, [](gpointer p) { delete static_cast<MacAccessMap *>(p); });
  g_signal_connect(page, "map", G_CALLBACK((+[](GtkWidget *widget, gpointer) {
                     if (!MacOsAccessibility::ShouldRefreshOnShow()) {
                       return;
                     }
                     auto *state = static_cast<MacAccessMap *>(g_object_get_data(G_OBJECT(widget), "macos-access"));
                     if (!state || !state->group) {
                       return;
                     }
                     const bool enabled = state->app && state->app->shortcuts() ? state->app->shortcuts()->IsMacAccessibilityEnabled() : false;
                     gtk_widget_set_visible(state->group, MacOsAccessibility::ShouldShowAccessRow(MacOsAccessibility::IsMacOs(), enabled));
                   })),
                   nullptr);

  return page;
}
