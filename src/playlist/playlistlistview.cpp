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
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble y, gpointer data) {
                     auto *self = static_cast<PlaylistListView *>(data);
                     if (GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(self->list_), static_cast<int>(y))) {
                       if (!gtk_list_box_row_is_selected(row)) {
                         gtk_list_box_unselect_all(GTK_LIST_BOX(self->list_));
                         gtk_list_box_select_row(GTK_LIST_BOX(self->list_), row);
                       }
                     }
                     if (self->menu_) {
                       self->menu_(self->SelectedName());
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
}

void PlaylistListView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void PlaylistListView::Refresh(const std::vector<PlaylistListDrop::Row> &rows, const std::string &current) {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  for (const PlaylistListDrop::Row &item : rows) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(PlaylistListDrop::DisplayName(item.name, item.favorite).c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data_full(G_OBJECT(row), "playlist-name", g_strdup(item.name.c_str()), g_free);
    if (item.name == current) {
      gtk_widget_add_css_class(row, "accent");
    }
    SetupRowDrop(row, item.name);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void PlaylistListView::SetupRowDrop(GtkWidget *row, const std::string &name) {
  GtkDropTarget *target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_drop_target_set_actions(target, static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE));
  g_object_set_data_full(G_OBJECT(target), "playlist-name", g_strdup(name.c_str()), g_free);
  g_signal_connect(target, "drop", G_CALLBACK((+[](GtkDropTarget *t, const GValue *value, gdouble, gdouble, gpointer data) -> gboolean {
                     auto *self = static_cast<PlaylistListView *>(data);
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(t), "playlist-name"));
                     if (!self->drop_ || !name || !G_VALUE_HOLDS_STRING(value)) {
                       return FALSE;
                     }
                     const char *text = g_value_get_string(value);
                     if (!text || !*text) {
                       return FALSE;
                     }
                     self->drop_(name, text);
                     return TRUE;
                   })),
                   this);
  gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(target));
}

std::string PlaylistListView::SelectedName() const {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  if (!row) {
    return {};
  }
  const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-name"));
  return name ? name : "";
}
