#ifndef STRAWBERRY_SMARTPLAYLISTWIZARD_H
#define STRAWBERRY_SMARTPLAYLISTWIZARD_H

#include <gtk/gtk.h>

class Application;

class SmartPlaylistWizard {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
