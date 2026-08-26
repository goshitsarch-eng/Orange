#ifndef STRAWBERRY_EDITTAGDIALOG_H
#define STRAWBERRY_EDITTAGDIALOG_H

#include <gtk/gtk.h>

class Application;

class EditTagDialog {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
