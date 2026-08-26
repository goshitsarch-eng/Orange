#ifndef STRAWBERRY_DELETECONFIRMATIONDIALOG_H
#define STRAWBERRY_DELETECONFIRMATIONDIALOG_H

#include <gtk/gtk.h>

class Application;

class DeleteConfirmationDialog {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
