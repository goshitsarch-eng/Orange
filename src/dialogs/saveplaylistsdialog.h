#ifndef STRAWBERRY_SAVEPLAYLISTSDIALOG_H
#define STRAWBERRY_SAVEPLAYLISTSDIALOG_H

#include <gtk/gtk.h>

class Application;

class SavePlaylistsDialog {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
