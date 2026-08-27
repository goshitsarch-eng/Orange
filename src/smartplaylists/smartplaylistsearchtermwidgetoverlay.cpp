#include "smartplaylists/smartplaylistsearchtermwidgetoverlay.h"

void SmartPlaylistSearchTermWidgetOverlay::Apply(GtkWidget *widget) {
  if (widget) {
    gtk_widget_add_css_class(widget, "smartplaylist-term");
    gtk_widget_add_css_class(widget, "smartplaylist-term-overlay");
  }
}
