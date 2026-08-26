#include "fileview/fileviewlist.h"

#include "fileview/fileviewdrag.h"
#include "utilities/fileutils.h"

#include <algorithm>

FileViewList::FileViewList() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  gtk_widget_set_hexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<FileViewList *>(data);
                     const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "file-path"));
                     if (path && self->activate_) {
                       self->activate_(path);
                     }
                   }),
                   this);
  GtkGesture *gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(gesture));
  g_signal_connect(gesture, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble, gdouble, gpointer data) {
                     auto *self = static_cast<FileViewList *>(data);
                     if (self->menu_) {
                       self->menu_(self->SelectedPaths());
                     }
                   }),
                   this);
}

void FileViewList::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void FileViewList::SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }

void FileViewList::Reload(const std::vector<std::string> &paths) {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  for (const std::string &path : paths) {
    GtkWidget *row = gtk_list_box_row_new();
    const bool dir = FileUtils::IsDirectory(path);
    GtkWidget *label = gtk_label_new(((dir ? "📁 " : "🎵 ") + FileUtils::BaseName(path)).c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data_full(G_OBJECT(row), "file-path", g_strdup(path.c_str()), g_free);
    SetupRowDrag(row, path);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void FileViewList::SetupRowDrag(GtkWidget *row, const std::string &path) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  g_object_set_data_full(G_OBJECT(src), "file-path", g_strdup(path.c_str()), g_free);
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<FileViewList *>(data);
                     const char *dragged = static_cast<const char *>(g_object_get_data(G_OBJECT(s), "file-path"));
                     std::vector<std::string> paths = self->SelectedPaths();
                     if (!dragged) {
                       return nullptr;
                     }
                     if (std::find(paths.begin(), paths.end(), dragged) == paths.end()) {
                       paths = {dragged};
                     }
                     const std::string payload = FileViewDrag::DragPayload(paths);
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

std::vector<std::string> FileViewList::SelectedPaths() const {
  std::vector<std::string> paths;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "file-path"));
        if (path) {
          static_cast<std::vector<std::string> *>(data)->emplace_back(path);
        }
      },
      &paths);
  return paths;
}
