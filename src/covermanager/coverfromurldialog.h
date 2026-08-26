#ifndef STRAWBERRY_COVERFROMURLDIALOG_H
#define STRAWBERRY_COVERFROMURLDIALOG_H

#include <gtk/gtk.h>

class Application;

class CoverFromUrlDialog {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
