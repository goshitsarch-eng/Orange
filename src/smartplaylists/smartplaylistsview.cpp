#include "smartplaylists/smartplaylistsview.h"

#include "translations/translations.h"

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
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *label = gtk_label_new(item.title.c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_box_append(GTK_BOX(box), label);
    auto *copy = new SmartPlaylistsItem(item);
    if (item.kind == SmartPlaylistsItem::Kind::Saved) {
      GtkWidget *remove = gtk_button_new_from_icon_name("edit-delete-symbolic");
      gtk_widget_set_tooltip_text(remove, Translations::CStr("Delete"));
      gtk_widget_set_valign(remove, GTK_ALIGN_CENTER);
      g_object_set_data(G_OBJECT(remove), "item", copy);
      g_signal_connect(remove, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                         auto *self = static_cast<SmartPlaylistsView *>(data);
                         auto *saved = static_cast<SmartPlaylistsItem *>(g_object_get_data(G_OBJECT(button), "item"));
                         if (saved && self->delete_) {
                           self->delete_(*saved);
                         }
                       }),
                       this);
      gtk_box_append(GTK_BOX(box), remove);
    }
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    g_object_set_data_full(G_OBJECT(row), "item", copy, [](gpointer p) { delete static_cast<SmartPlaylistsItem *>(p); });
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}
