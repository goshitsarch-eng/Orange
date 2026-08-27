#ifndef STRAWBERRY_ADDSTREAMDIALOG_H
#define STRAWBERRY_ADDSTREAMDIALOG_H

#include <functional>
#include <string>

#include <gtk/gtk.h>

class AddStreamDialog {
 public:
  static void Show(GtkWindow *parent, const std::function<void(const std::string &, const std::string &)> &callback);
};

#endif
