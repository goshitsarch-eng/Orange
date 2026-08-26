#ifndef STRAWBERRY_SETTINGSDIALOG_H
#define STRAWBERRY_SETTINGSDIALOG_H

#include <gtk/gtk.h>

class Application;

class SettingsDialog {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
