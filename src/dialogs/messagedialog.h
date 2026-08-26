#ifndef STRAWBERRY_MESSAGEDIALOG_H
#define STRAWBERRY_MESSAGEDIALOG_H

#include <functional>
#include <string>

#include <gtk/gtk.h>

class MessageDialog {
 public:
  static void Show(GtkWindow *parent, const std::string &title, const std::string &message);
  static void Show(GtkWindow *parent, const std::string &title, const std::string &message, const std::string &checkbox_label,
                   bool checkbox_checked, const std::function<void(bool checked)> &closed);
};

#endif
