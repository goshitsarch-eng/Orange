#ifndef STRAWBERRY_TRACKSELECTIONDIALOG_H
#define STRAWBERRY_TRACKSELECTIONDIALOG_H

#include <gtk/gtk.h>

class Application;

class TrackSelectionDialog {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
