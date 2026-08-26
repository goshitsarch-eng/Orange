#include "fileview/fileviewtree.h"

#include "fileview/fileviewdrag.h"

#include <string>
#include <vector>

FileViewTree::FileViewTree() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  gtk_widget_set_size_request(widget_, 180, -1);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "navigation-sidebar");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<FileViewTree *>(data);
                     const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "file-path"));
                     if (path && self->activate_) {
                       self->activate_(path);
                     }
                   }),
                   this);
  GtkGesture *gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(gesture));
  g_signal_connect(gesture, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble y, gpointer data) {
                     auto *self = static_cast<FileViewTree *>(data);
                     if (!self->menu_) {
                       return;
                     }
                     GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(self->list_), static_cast<int>(y));
                     if (row && !gtk_list_box_row_is_selected(row)) {
                       gtk_list_box_unselect_all(GTK_LIST_BOX(self->list_));
                       gtk_list_box_select_row(GTK_LIST_BOX(self->list_), row);
                     }
                     const char *path = row ? static_cast<const char *>(g_object_get_data(G_OBJECT(row), "file-path")) : nullptr;
                     if (path) {
                       self->menu_(path);
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
}

void FileViewTree::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void FileViewTree::SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }

void FileViewTree::AppendItem(GtkWidget *parent, FileViewTreeItem *item, int depth) {
  if (!item) {
    return;
  }
  if (!item->path.empty()) {
    GtkWidget *row = gtk_list_box_row_new();
    const char *prefix = item->type == FileViewTreeItem::Type::File ? "🎵 " : "📁 ";
    GtkWidget *label = gtk_label_new((std::string(prefix) + item->name).c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12 + depth * 16);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 6);
    gtk_widget_set_margin_bottom(label, 6);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data_full(G_OBJECT(row), "file-path", g_strdup(item->path.c_str()), g_free);
    SetupRowDrag(row, item->path);
    gtk_list_box_append(GTK_LIST_BOX(parent), row);
  }
  for (const auto &child : item->children) {
    AppendItem(parent, child.get(), item->path.empty() ? depth : depth + 1);
  }
}

void FileViewTree::Reload(FileViewTreeModel *model) {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  if (!model || !model->root()) {
    return;
  }
  for (const auto &item : model->root()->children) {
    model->LazyLoad(item.get());
    AppendItem(list_, item.get(), 0);
  }
}

void FileViewTree::SetupRowDrag(GtkWidget *row, const std::string &path) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  g_object_set_data_full(G_OBJECT(src), "file-path", g_strdup(path.c_str()), g_free);
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<FileViewTree *>(data);
                     const char *dragged = static_cast<const char *>(g_object_get_data(G_OBJECT(s), "file-path"));
                     const std::string selected = self->SelectedPath();
                     const std::vector<std::string> selected_paths = selected.empty() ? std::vector<std::string>{} : std::vector<std::string>{selected};
                     const std::string payload = FileViewDrag::DragPayload(FileViewDrag::PathsForDrag(selected_paths, dragged ? dragged : ""));
                     if (payload.empty()) {
                       return nullptr;
                     }
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

std::string FileViewTree::SelectedPath() const {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  const char *path = row ? static_cast<const char *>(g_object_get_data(G_OBJECT(row), "file-path")) : nullptr;
  return path ? path : std::string();
}
