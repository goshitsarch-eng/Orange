#ifndef STRAWBERRY_USERPASSDIALOG_H
#define STRAWBERRY_USERPASSDIALOG_H

#include <functional>
#include <string>

#include <gtk/gtk.h>

class UserPassDialog {
 public:
  static void Show(GtkWindow *parent, const std::string &service, const std::function<void(const std::string &, const std::string &)> &callback);
};

#endif
