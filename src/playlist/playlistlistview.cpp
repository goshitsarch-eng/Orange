#include "playlist/playlistlistview.h"

#include "translations/translations.h"

PlaylistListView::PlaylistListView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<PlaylistListView *>(data);
                     const gboolean folder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "playlist-folder")) == 1;
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-name"));
                     const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-path"));
                     if (folder) {
                       if (path && self->toggle_) {
                         self->toggle_(path);
                       }
                       return;
                     }
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
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(outer, 8 + item.depth * 12);
    gtk_widget_set_margin_end(outer, 8);
    gtk_widget_set_margin_top(outer, 4);
    gtk_widget_set_margin_bottom(outer, 4);
    if (item.folder) {
      GtkWidget *toggle = gtk_button_new_from_icon_name(item.expanded ? "pan-down-symbolic" : "pan-end-symbolic");
      gtk_widget_add_css_class(toggle, "flat");
      gtk_widget_add_css_class(toggle, "circular");
      gtk_widget_set_tooltip_text(toggle, item.expanded ? Translations::CStr("Collapse") : Translations::CStr("Expand"));
      g_object_set_data_full(G_OBJECT(toggle), "playlist-path", g_strdup(item.path.c_str()), g_free);
      g_signal_connect(toggle, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                         auto *self = static_cast<PlaylistListView *>(data);
                         const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(button), "playlist-path"));
                         if (path && self->toggle_) {
                           self->toggle_(path);
                         }
                       }),
                       this);
      gtk_box_append(GTK_BOX(outer), toggle);
      GtkWidget *icon = gtk_image_new_from_icon_name("folder-symbolic");
      gtk_box_append(GTK_BOX(outer), icon);
    }
    GtkWidget *label = gtk_label_new(item.folder ? item.name.c_str() : PlaylistListDrop::DisplayName(item.name, item.favorite).c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_box_append(GTK_BOX(outer), label);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), outer);
    g_object_set_data_full(G_OBJECT(row), "playlist-name", g_strdup(item.folder ? "" : item.name.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(row), "playlist-path", g_strdup(item.path.c_str()), g_free);
    g_object_set_data(G_OBJECT(row), "playlist-folder", GINT_TO_POINTER(item.folder ? 1 : 2));
    if (!item.folder && item.name == current) {
      gtk_widget_add_css_class(row, "accent");
    }
    SetupRowDrop(row, item);
    if (!item.folder) {
      SetupRowDrag(row, item.name);
    }
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void PlaylistListView::SetupRowDrop(GtkWidget *row, const PlaylistListDrop::Row &item) {
  GtkDropTarget *target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_drop_target_set_actions(target, static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE));
  g_object_set_data_full(G_OBJECT(target), "playlist-name", g_strdup(item.folder ? item.path.c_str() : item.name.c_str()), g_free);
  g_object_set_data(G_OBJECT(target), "playlist-folder", GINT_TO_POINTER(item.folder ? 1 : 2));
  g_signal_connect(target, "drop", G_CALLBACK((+[](GtkDropTarget *t, const GValue *value, gdouble, gdouble, gpointer data) -> gboolean {
                     auto *self = static_cast<PlaylistListView *>(data);
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(t), "playlist-name"));
                     const bool folder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(t), "playlist-folder")) == 1;
                     if (!self->drop_ || !name || !G_VALUE_HOLDS_STRING(value)) {
                       return FALSE;
                     }
                     const char *text = g_value_get_string(value);
                     if (!text || !*text) {
                       return FALSE;
                     }
                     self->drop_(name, text, folder);
                     return TRUE;
                   })),
                   this);
  gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(target));
}

void PlaylistListView::SetupRowDrag(GtkWidget *row, const std::string &name) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_MOVE);
  g_object_set_data_full(G_OBJECT(src), "playlist-name", g_strdup(name.c_str()), g_free);
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer) -> GdkContentProvider * {
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(s), "playlist-name"));
                     if (!name || !*name) {
                       return nullptr;
                     }
                     const std::string payload = PlaylistListDrop::MovePayload(name);
                     GValue v = G_VALUE_INIT;
                     g_value_init(&v, G_TYPE_STRING);
                     g_value_set_string(&v, payload.c_str());
                     GdkContentProvider *provider = gdk_content_provider_new_for_value(&v);
                     g_value_unset(&v);
                     return provider;
                   })),
                   this);
  gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(src));
}

std::string PlaylistListView::SelectedName() const {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  if (!row) {
    return {};
  }
  const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-name"));
  return name ? name : "";
}

std::string PlaylistListView::SelectedFolderPath() const {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  if (!row || GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "playlist-folder")) != 1) {
    return {};
  }
  const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-path"));
  return path ? path : "";
}

bool PlaylistListView::SelectedIsFolder() const { return !SelectedFolderPath().empty(); }
