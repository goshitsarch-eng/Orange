#include "settings/settingsitemdelegate.h"

#include <adwaita.h>

GtkWidget *SettingsItemDelegate::MakeRow(const std::string &title, const std::string &subtitle) {
  if (subtitle.empty()) {
    return GTK_WIDGET(adw_action_row_new());
  }
  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title.c_str());
  adw_action_row_set_subtitle(row, subtitle.c_str());
  return GTK_WIDGET(row);
}
