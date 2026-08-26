#ifndef STRAWBERRY_LISTBOXKEYBOARDGTK_H
#define STRAWBERRY_LISTBOXKEYBOARDGTK_H

#include "widgets/listboxkeyboard.h"

#include <gtk/gtk.h>

#include <string>
#include <vector>

namespace ListBoxKeyboardGtk {

inline int Count(GtkWidget *list) {
  int count = 0;
  if (!list) {
    return count;
  }
  for (GtkWidget *child = gtk_widget_get_first_child(list); child; child = gtk_widget_get_next_sibling(child)) {
    if (GTK_IS_LIST_BOX_ROW(child)) {
      ++count;
    }
  }
  return count;
}

inline int SelectedIndex(GtkWidget *list) {
  if (!GTK_IS_LIST_BOX(list)) {
    return -1;
  }
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list));
  return row ? gtk_list_box_row_get_index(row) : -1;
}

inline void SelectIndex(GtkWidget *list, int index) {
  if (!GTK_IS_LIST_BOX(list) || index < 0) {
    return;
  }
  GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(list), index);
  if (!row) {
    return;
  }
  gtk_list_box_unselect_all(GTK_LIST_BOX(list));
  gtk_list_box_select_row(GTK_LIST_BOX(list), row);
  gtk_widget_grab_focus(GTK_WIDGET(row));
}

inline std::vector<std::string> Labels(GtkWidget *list) {
  std::vector<std::string> labels;
  if (!list) {
    return labels;
  }
  for (GtkWidget *child = gtk_widget_get_first_child(list); child; child = gtk_widget_get_next_sibling(child)) {
    if (!GTK_IS_LIST_BOX_ROW(child)) {
      continue;
    }
    GtkWidget *inner = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(child));
    if (GTK_IS_LABEL(inner)) {
      labels.emplace_back(gtk_label_get_text(GTK_LABEL(inner)));
      continue;
    }
    std::string text;
    for (GtkWidget *label = inner ? gtk_widget_get_first_child(inner) : nullptr; label; label = gtk_widget_get_next_sibling(label)) {
      if (GTK_IS_LABEL(label)) {
        text = gtk_label_get_text(GTK_LABEL(label));
        break;
      }
    }
    labels.push_back(text);
  }
  return labels;
}

inline void ActivateSelected(GtkWidget *list) {
  if (!GTK_IS_LIST_BOX(list)) {
    return;
  }
  if (GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list))) {
    gtk_widget_activate(GTK_WIDGET(row));
  }
}

}  // namespace ListBoxKeyboardGtk

#endif
