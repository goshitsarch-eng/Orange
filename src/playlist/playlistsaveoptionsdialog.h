#ifndef STRAWBERRY_PLAYLISTSAVEOPTIONSDIALOG_H
#define STRAWBERRY_PLAYLISTSAVEOPTIONSDIALOG_H

#include "constants/playlistsettings.h"

#include <gtk/gtk.h>

#include <functional>

class PlaylistSaveOptionsDialog {
 public:
  using PathType = PlaylistSettings::PathType;

  static void Show(GtkWindow *parent, const std::function<void(PathType)> &callback);
  static const char *Label(PathType type);
};

#endif
