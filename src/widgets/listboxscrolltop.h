#ifndef STRAWBERRY_LISTBOXSCROLLTOP_H
#define STRAWBERRY_LISTBOXSCROLLTOP_H

#include "collection/collectiontypeaheadscroll.h"

#include <gtk/gtk.h>

namespace ListBoxScrollTop {

inline GtkWidget *ScrolledAncestor(GtkWidget *widget) {
  return widget ? gtk_widget_get_ancestor(widget, GTK_TYPE_SCROLLED_WINDOW) : nullptr;
}

inline void RowToTop(GtkWidget *list, GtkWidget *row) {
  GtkWidget *scrolled = ScrolledAncestor(list);
  if (!scrolled || !list || !row) {
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
  gtk_adjustment_set_value(adjust, CollectionTypeAheadScroll::PositionAtTop(bounds.origin.y, gtk_adjustment_get_lower(adjust),
                                                                            gtk_adjustment_get_upper(adjust),
                                                                            gtk_adjustment_get_page_size(adjust)));
}

inline void SelectedRowToTop(GtkWidget *list) {
  if (!GTK_IS_LIST_BOX(list)) {
    return;
  }
  if (GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list))) {
    RowToTop(list, GTK_WIDGET(row));
  }
}

}  // namespace ListBoxScrollTop

#endif
