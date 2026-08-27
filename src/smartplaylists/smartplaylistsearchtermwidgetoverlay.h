#ifndef STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGETOVERLAY_H
#define STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGETOVERLAY_H

#include <gtk/gtk.h>

class SmartPlaylistSearchTermWidgetOverlay {
 public:
  static const char *Label() { return "Add search term"; }
  static void Apply(GtkWidget *widget);
};

#endif
