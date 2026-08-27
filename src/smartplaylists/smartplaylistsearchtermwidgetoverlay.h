#ifndef STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGETOVERLAY_H
#define STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGETOVERLAY_H

#include <gtk/gtk.h>

class SmartPlaylistSearchTermWidgetOverlay {
 public:
  static const char *Label() { return "Add search term"; }
  static void Apply(GtkWidget *widget);

  // Qt SmartPlaylistSearchTermWidget::showEvent calls overlay Grab() when inactive.
  static bool ShouldGrabOnShow(bool active) { return !active; }

  // Qt overlay keyReleaseEvent accepts Space (and Return is the GTK default activate).
  static bool IsActivateKey(unsigned keyval) { return keyval == 0x0020 || keyval == 0xff0d; }
};

#endif
