#ifndef STRAWBERRY_ORGANIZEDIALOG_H
#define STRAWBERRY_ORGANIZEDIALOG_H

#include <gtk/gtk.h>

class Application;

class OrganizeDialog {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
