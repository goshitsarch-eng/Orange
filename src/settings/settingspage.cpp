#include "settings/settingspage.h"

#include <string>

namespace SettingsPage {

AdwPreferencesPage *MakePage(const char *name, const char *icon) {
  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
  adw_preferences_page_set_title(page, name);
  adw_preferences_page_set_icon_name(page, icon);
  return page;
}

AdwPreferencesGroup *AddGroup(AdwPreferencesPage *page, const char *title) {
  AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
  if (title && title[0]) {
    adw_preferences_group_set_title(group, title);
  }
  adw_preferences_page_add(page, group);
  return group;
}

void AddToggle(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *subtitle, bool fallback) {
  AdwSwitchRow *row = ADW_SWITCH_ROW(adw_switch_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
  if (subtitle) {
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), subtitle);
  }
  adw_switch_row_set_active(row, settings->BoolValue(key, fallback));
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  g_signal_connect(row, "notify::active", G_CALLBACK(+[](AdwSwitchRow *switch_row, GParamSpec *, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(switch_row), "settings-key"));
                     s->SetBoolValue(settings_key, adw_switch_row_get_active(switch_row));
                     s->Sync();
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

void AddEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *fallback) {
  AdwEntryRow *row = ADW_ENTRY_ROW(adw_entry_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
  gtk_editable_set_text(GTK_EDITABLE(row), settings->Value(key, fallback ? fallback : "").c_str());
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  g_signal_connect(row, "changed", G_CALLBACK(+[](AdwEntryRow *entry, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(entry), "settings-key"));
                     s->SetValue(settings_key, gtk_editable_get_text(GTK_EDITABLE(entry)));
                     s->Sync();
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

void AddIntEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, int fallback) {
  AdwEntryRow *row = ADW_ENTRY_ROW(adw_entry_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
  gtk_editable_set_text(GTK_EDITABLE(row), std::to_string(settings->IntValue(key, fallback)).c_str());
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  g_signal_connect(row, "changed", G_CALLBACK(+[](AdwEntryRow *entry, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(entry), "settings-key"));
                     s->SetIntValue(settings_key, g_ascii_strtoll(gtk_editable_get_text(GTK_EDITABLE(entry)), nullptr, 10));
                     s->Sync();
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

void AddButtonRow(AdwPreferencesGroup *group, const char *title, const char *button_label, const std::function<void()> &clicked) {
  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
  GtkWidget *button = gtk_button_new_with_label(button_label);
  auto *fn = new std::function<void()>(clicked);
  g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     (*static_cast<std::function<void()> *>(data))();
                   }),
                   fn);
  g_object_set_data_full(G_OBJECT(button), "clicked-fn", fn, +[](gpointer data) { delete static_cast<std::function<void()> *>(data); });
  adw_action_row_add_suffix(row, button);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

}  // namespace SettingsPage
