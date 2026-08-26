#ifndef STRAWBERRY_MESSAGEDIALOG_H
#define STRAWBERRY_MESSAGEDIALOG_H

#include <string>

#include <gtk/gtk.h>

class MessageDialog {
 public:
  static void Show(GtkWindow *parent, const std::string &title, const std::string &message);
};

#endif
