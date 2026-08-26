#include "playlist/playlistlistview.h"

PlaylistListView::PlaylistListView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<PlaylistListView *>(data);
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-name"));
                     if (name && self->activate_) {
                       self->activate_(name);
                     }
                   }),
                   this);
}

void PlaylistListView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void PlaylistListView::Refresh(const PlaylistListModel &model) {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  for (int i = 0; i < model.Count(); ++i) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(model.At(i).c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data_full(G_OBJECT(row), "playlist-name", g_strdup(model.At(i).c_str()), g_free);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}
