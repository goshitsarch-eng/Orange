#ifndef STRAWBERRY_ALBUMCOVERMANAGER_H
#define STRAWBERRY_ALBUMCOVERMANAGER_H

#include <gtk/gtk.h>

class Application;

class AlbumCoverManager {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
