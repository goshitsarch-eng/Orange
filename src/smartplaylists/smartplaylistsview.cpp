#include "smartplaylists/smartplaylistsview.h"

SmartPlaylistsView::SmartPlaylistsView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<SmartPlaylistsView *>(data);
                     auto *item = static_cast<SmartPlaylistsItem *>(g_object_get_data(G_OBJECT(row), "item"));
                     if (item && self->activate_) {
                       self->activate_(*item);
                     }
                   }),
                   this);
}

void SmartPlaylistsView::Reload(SmartPlaylistsModel *model) {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  if (!model) {
    return;
  }
  for (const SmartPlaylistsItem &item : model->items()) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(item.title.c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    auto *copy = new SmartPlaylistsItem(item);
    g_object_set_data_full(G_OBJECT(row), "item", copy, [](gpointer p) { delete static_cast<SmartPlaylistsItem *>(p); });
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}
