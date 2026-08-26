#ifndef STRAWBERRY_SETTINGSITEMDELEGATE_H
#define STRAWBERRY_SETTINGSITEMDELEGATE_H

#include <gtk/gtk.h>

#include <string>

class SettingsItemDelegate {
 public:
  static GtkWidget *MakeRow(const std::string &title, const std::string &subtitle = {});
};

#endif
