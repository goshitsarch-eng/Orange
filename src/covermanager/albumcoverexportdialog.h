#ifndef STRAWBERRY_ALBUMCOVEREXPORTDIALOG_H
#define STRAWBERRY_ALBUMCOVEREXPORTDIALOG_H

#include "core/song.h"

#include <gtk/gtk.h>

class Application;

class AlbumCoverExportDialog {
 public:
  static void Show(GtkWindow *parent, Application *app, const SongList &songs = {});
};

#endif
