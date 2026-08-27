#ifndef STRAWBERRY_LISTBOXENSUREVISIBLE_H
#define STRAWBERRY_LISTBOXENSUREVISIBLE_H

#include "playlist/playlistlistscroll.h"

#include <gtk/gtk.h>

namespace ListBoxEnsureVisible {

inline void Row(GtkWidget *list, GtkWidget *row) {
  GtkWidget *scrolled = list ? gtk_widget_get_ancestor(list, GTK_TYPE_SCROLLED_WINDOW) : nullptr;
  if (!scrolled || !row) {
    return;
  }
  graphene_rect_t bounds;
  if (!gtk_widget_compute_bounds(row, list, &bounds)) {
    return;
  }
  GtkAdjustment *adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled));
  if (!adjust) {
    return;
  }
  const double value = gtk_adjustment_get_value(adjust);
  const double page = gtk_adjustment_get_page_size(adjust);
  const PlaylistListScroll::Hint hint = PlaylistListScroll::FromBounds(bounds.origin.y, bounds.size.height, value, page);
  gtk_adjustment_set_value(adjust, PlaylistListScroll::Value(hint, bounds.origin.y, bounds.size.height, value, page,
                                                             gtk_adjustment_get_lower(adjust), gtk_adjustment_get_upper(adjust)));
}

}  // namespace ListBoxEnsureVisible

#endif
