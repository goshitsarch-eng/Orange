#include "playlist/playlistproxystyle.h"

void PlaylistProxyStyle::Apply(GtkWidget *widget) {
  if (widget) {
    gtk_widget_add_css_class(widget, "playlist");
  }
}
