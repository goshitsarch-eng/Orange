#ifndef STRAWBERRY_SMARTPLAYLISTWIZARD_H
#define STRAWBERRY_SMARTPLAYLISTWIZARD_H

#include "smartplaylists/smartplaylist.h"

#include <gtk/gtk.h>

#include <string>

class Application;

class SmartPlaylistWizard {
 public:
  static void Show(GtkWindow *parent, Application *app);
  static void Show(GtkWindow *parent, Application *app, const std::string &name, const SmartPlaylistSearch &search);
};

#endif
