#ifndef STRAWBERRY_ERRORDIALOG_H
#define STRAWBERRY_ERRORDIALOG_H

#include <string>

#include <gtk/gtk.h>

class ErrorDialog {
 public:
  static void Show(GtkWindow *parent, const std::string &message);
};

#endif
