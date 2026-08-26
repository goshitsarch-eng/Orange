#ifndef STRAWBERRY_ALBUMCOVERSEARCHER_H
#define STRAWBERRY_ALBUMCOVERSEARCHER_H

#include <gtk/gtk.h>

class Application;

class AlbumCoverSearcher {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
