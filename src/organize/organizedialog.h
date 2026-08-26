#ifndef STRAWBERRY_ORGANIZEDIALOG_H
#define STRAWBERRY_ORGANIZEDIALOG_H

#include "core/song.h"

#include <gtk/gtk.h>

class Application;

class OrganizeDialog {
 public:
  static void Show(GtkWindow *parent, Application *app, const SongList &songs = {});
};

#endif
