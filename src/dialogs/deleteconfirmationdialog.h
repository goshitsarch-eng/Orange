#ifndef STRAWBERRY_DELETECONFIRMATIONDIALOG_H
#define STRAWBERRY_DELETECONFIRMATIONDIALOG_H

#include "core/song.h"
#include "dialogs/deletefilespolicy.h"

#include <gtk/gtk.h>

class Application;

class DeleteConfirmationDialog {
 public:
  static void Show(GtkWindow *parent, Application *app, const SongList &songs = {},
                   DeleteFilesPolicy::Source source = DeleteFilesPolicy::Source::Playlist);
};

#endif
