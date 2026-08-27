#ifndef STRAWBERRY_ORGANIZEERRORDIALOG_H
#define STRAWBERRY_ORGANIZEERRORDIALOG_H

#include "core/song.h"
#include "organize/organize.h"

#include <gtk/gtk.h>

class OrganizeErrorDialog {
 public:
  enum class OperationType { Copy, Delete };

  static const char *Title(OperationType type) {
    return type == OperationType::Delete ? "Error deleting songs" : "Error copying songs";
  }

  static const char *Message(OperationType type) {
    return type == OperationType::Delete ? "There were problems deleting some songs.  The following files could not be deleted:"
                                         : "There were problems copying some songs.  The following files could not be copied:";
  }

  static void Show(GtkWindow *parent, const std::vector<Organize::Error> &errors);
  static void Show(GtkWindow *parent, OperationType type, const SongList &songs);
};

#endif
