#ifndef STRAWBERRY_COPYTODEVICEDIALOG_H
#define STRAWBERRY_COPYTODEVICEDIALOG_H

#include "core/song.h"

#include <gtk/gtk.h>

#include <string>
#include <vector>

class Application;

class CopyToDeviceDialog {
 public:
  static void Show(GtkWindow *parent, Application *app, const SongList &songs = {}, const std::string &playlist = {},
                   const std::vector<std::string> &filenames = {});
};

#endif
