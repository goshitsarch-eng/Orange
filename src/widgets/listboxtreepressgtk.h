#ifndef LISTBOXTREEPRESGTK_H
#define LISTBOXTREEPRESGTK_H

#include "config.h"

#include "collection/collectiontreeclick.h"

#include <gtk/gtk.h>

namespace ListBoxTreePressGtk {

inline GtkListBoxRow *RowAtY(GtkWidget *list, const double y) {
  return list ? gtk_list_box_get_row_at_y(GTK_LIST_BOX(list), static_cast<int>(y)) : nullptr;
}

inline bool OnExpandControl(GtkWidget *list, const double x, const double y) {
  if (!list) {
    return false;
  }
  GtkWidget *picked = gtk_widget_pick(list, x, y, GTK_PICK_DEFAULT);
  return picked && gtk_widget_get_ancestor(picked, GTK_TYPE_BUTTON);
}

inline void SelectRowIfNeeded(GtkWidget *list, GtkListBoxRow *row) {
  if (!list || !row || gtk_list_box_row_is_selected(row)) {
    return;
  }
  gtk_list_box_unselect_all(GTK_LIST_BOX(list));
  gtk_list_box_select_row(GTK_LIST_BOX(list), row);
}

template <typename T>
inline void Attach(GtkWidget *list, T *self) {
  if (!list || !self) {
    return;
  }
  GtkGesture *primary = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(primary), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller(list, GTK_EVENT_CONTROLLER(primary));
  g_signal_connect(primary, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint n_press, gdouble x, gdouble y, gpointer data) {
                     static_cast<T *>(data)->HandlePress(GDK_BUTTON_PRIMARY, n_press, x, y,
                                                         gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(click)));
                   }),
                   self);

  GtkGesture *middle = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(middle), GDK_BUTTON_MIDDLE);
  gtk_widget_add_controller(list, GTK_EVENT_CONTROLLER(middle));
  g_signal_connect(middle, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint n_press, gdouble x, gdouble y, gpointer data) {
                     static_cast<T *>(data)->HandlePress(GDK_BUTTON_MIDDLE, n_press, x, y,
                                                         gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(click)));
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   self);
}

}  // namespace ListBoxTreePressGtk

#endif  // LISTBOXTREEPRESGTK_H
