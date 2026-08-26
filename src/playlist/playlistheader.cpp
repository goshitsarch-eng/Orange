#include "playlist/playlistheader.h"

PlaylistHeader::PlaylistHeader() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(widget_, "toolbar");
}

void PlaylistHeader::Rebuild(const SortCallback &callback) {
  GtkWidget *child = gtk_widget_get_first_child(widget_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_widget_unparent(child);
    child = next;
  }
  auto *cb = new SortCallback(callback);
  g_object_set_data_full(G_OBJECT(widget_), "sort-callback", cb, [](gpointer p) { delete static_cast<SortCallback *>(p); });
  for (int i = 0; i < static_cast<int>(PlaylistColumn::Count); ++i) {
    const auto column = static_cast<PlaylistColumn>(i);
    if (!PlaylistDelegates::ColumnVisible(column)) {
      continue;
    }
    GtkWidget *button = gtk_button_new_with_label(PlaylistDelegates::ColumnTitle(column).c_str());
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_hexpand(button, column == PlaylistColumn::Title);
    gtk_widget_set_size_request(button, PlaylistDelegates::ColumnWidth(column), -1);
    g_object_set_data(G_OBJECT(button), "column", GINT_TO_POINTER(i));
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *fn = static_cast<SortCallback *>(data);
                       if (fn && *fn) {
                         (*fn)(static_cast<PlaylistColumn>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "column"))));
                       }
                     }),
                     cb);
    gtk_box_append(GTK_BOX(widget_), button);
  }
}
