#ifndef STRAWBERRY_PLAYLISTSAVEOPTIONSDIALOG_H
#define STRAWBERRY_PLAYLISTSAVEOPTIONSDIALOG_H

#include <gtk/gtk.h>

#include <functional>
#include <string>

class PlaylistSaveOptionsDialog {
 public:
  enum class PathType {
    Automatic = 0,
    Relative = 1,
    Absolute = 2
  };

  static void Show(GtkWindow *parent, const std::function<void(PathType)> &callback);
  static const char *Label(PathType type);
};

#endif
