#ifndef STRAWBERRY_SETTINGSDIALOG_H
#define STRAWBERRY_SETTINGSDIALOG_H

#include <functional>

#include <gtk/gtk.h>

class Application;

class SettingsDialog {
 public:
  static void Show(GtkWindow *parent, Application *app, const std::function<void()> &closed = {}, const char *page_name = nullptr);
};

#endif
