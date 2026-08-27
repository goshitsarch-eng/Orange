#ifndef STRAWBERRY_PLAYLISTCOLUMNSDIALOG_H
#define STRAWBERRY_PLAYLISTCOLUMNSDIALOG_H

#include <functional>

#include <gtk/gtk.h>

class PlaylistColumnsDialog {
 public:
  static void Show(GtkWindow *parent, const std::function<void()> &callback);
};

#endif
