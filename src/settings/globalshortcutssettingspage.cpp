#include "settings/globalshortcutssettingspage.h"

#include "constants/globalshortcutssettings.h"
#include "core/application.h"
#include "core/settings.h"
#include "globalshortcuts/globalshortcutbinding.h"
#include "globalshortcuts/globalshortcuts.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "ui/dialogs.h"

namespace {

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
  SettingsPage::AddToggle(backends, settings, GlobalShortcutsSettings::kUseKGlobalAccel, GlobalShortcutBinding::UseKGlobalAccel(), nullptr,
                          GlobalShortcutsSettings::kDefaultUseKGlobalAccel);
  GtkWidget *x11 = SettingsPage::AddToggle(backends, settings, GlobalShortcutsSettings::kUseX11, GlobalShortcutBinding::UseX11(), nullptr,
                                           GlobalShortcutsSettings::kDefaultUseX11);
  GtkWidget *warning = gtk_label_new(GlobalShortcutBinding::X11Warning());
  gtk_label_set_wrap(GTK_LABEL(warning), TRUE);
  gtk_label_set_xalign(GTK_LABEL(warning), 0.0f);
  gtk_widget_set_margin_start(warning, 12);
  gtk_widget_set_margin_end(warning, 12);
  gtk_widget_set_visible(warning, adw_switch_row_get_active(ADW_SWITCH_ROW(x11)));
  g_signal_connect(x11, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer data) {
                     gtk_widget_set_visible(GTK_WIDGET(data), adw_switch_row_get_active(row));
                   }),
                   warning);
  adw_preferences_group_add(backends, warning);

  AdwPreferencesGroup *keys = SettingsPage::AddGroup(page, "Shortcuts");
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
  return page;
}
