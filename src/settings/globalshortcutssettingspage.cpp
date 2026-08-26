#include "settings/globalshortcutssettingspage.h"

#include "constants/globalshortcutssettings.h"
#include "core/application.h"
#include "globalshortcuts/globalshortcuts.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "ui/dialogs.h"

AdwPreferencesPage *GlobalShortcutsSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Shortcuts", "input-keyboard-symbolic");
  AdwPreferencesGroup *backends = SettingsPage::AddGroup(page, "Backends");
  SettingsPage::AddToggle(backends, settings, "enabled", "Enable global shortcuts", nullptr, true);
  SettingsPage::AddToggle(backends, settings, GlobalShortcutsSettings::kUseKGlobalAccel, "Use KGlobalAccel", nullptr,
                          GlobalShortcutsSettings::kDefaultUseKGlobalAccel);
  SettingsPage::AddToggle(backends, settings, GlobalShortcutsSettings::kUseX11, "Use X11 grab", nullptr,
                          GlobalShortcutsSettings::kDefaultUseX11);

  AdwPreferencesGroup *keys = SettingsPage::AddGroup(page, "Shortcuts");
  for (const auto &def : GlobalShortcutsManager::Catalog()) {
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(def.description));
    GtkWidget *entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(entry), settings->Value(def.id, def.default_key).c_str());
    gtk_widget_set_size_request(entry, 140, -1);
    g_object_set_data_full(G_OBJECT(entry), "settings-key", g_strdup(def.id), g_free);
    g_signal_connect(entry, "changed", G_CALLBACK(+[](GtkEditable *editable, gpointer data) {
                       auto *s = static_cast<Settings *>(data);
                       const char *key = static_cast<const char *>(g_object_get_data(G_OBJECT(editable), "settings-key"));
                       s->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
                       s->SetValue(key, gtk_editable_get_text(editable));
                       s->Sync();
                     }),
                     settings);
    GtkWidget *grab = gtk_button_new_with_label(Translations::CStr("Grab"));
    g_object_set_data(G_OBJECT(grab), "settings", settings);
    g_object_set_data(G_OBJECT(grab), "entry", entry);
    g_object_set_data(G_OBJECT(grab), "app", app);
    g_object_set_data_full(G_OBJECT(grab), "settings-key", g_strdup(def.id), g_free);
    g_object_set_data_full(G_OBJECT(grab), "action-name", g_strdup(def.description), g_free);
    g_signal_connect(grab, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                       auto *s = static_cast<Settings *>(g_object_get_data(G_OBJECT(button), "settings"));
                       GtkWidget *key_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "entry"));
                       const char *key = static_cast<const char *>(g_object_get_data(G_OBJECT(button), "settings-key"));
                       auto *application = static_cast<Application *>(g_object_get_data(G_OBJECT(button), "app"));
                       const char *description = static_cast<const char *>(g_object_get_data(G_OBJECT(button), "action-name"));
                       Dialogs::GrabShortcut(
                           nullptr,
                           [s, key_entry, key, application](const std::string &accel) {
                             if (s && key) {
                               s->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
                               s->SetValue(key, accel);
                               s->Sync();
                             }
                             if (key_entry) {
                               gtk_editable_set_text(GTK_EDITABLE(key_entry), accel.c_str());
                             }
                             if (application) {
                               application->shortcuts()->ReloadSettings();
                             }
                           },
                           description ? description : "");
                     }),
                     nullptr);
    adw_action_row_add_suffix(row, entry);
    adw_action_row_add_suffix(row, grab);
    adw_preferences_group_add(keys, GTK_WIDGET(row));
  }
  return page;
}
