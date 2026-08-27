#include "widgets/autoexpandingtreeview.h"

AutoExpandingTreeView::AutoExpandingTreeView() {
  list_ = gtk_list_box_new();
  widget_ = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
}

void AutoExpandingTreeView::ExpandAll() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    if (GTK_IS_LIST_BOX_ROW(child)) {
      gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(child), TRUE);
    }
    child = gtk_widget_get_next_sibling(child);
  }
}

void AutoExpandingTreeView::AppendRow(const std::string &text) {
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *label = gtk_label_new(text.c_str());
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
  gtk_list_box_append(GTK_LIST_BOX(list_), row);
}

void AutoExpandingTreeView::Clear() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
}
