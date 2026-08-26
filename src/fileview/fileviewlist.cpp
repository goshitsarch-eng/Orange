#include "fileview/fileviewlist.h"

#include "utilities/fileutils.h"

FileViewList::FileViewList() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  gtk_widget_set_hexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
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
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

std::vector<std::string> FileViewList::SelectedPaths() const {
  std::vector<std::string> paths;
  if (GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_))) {
    const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "file-path"));
    if (path) {
      paths.emplace_back(path);
    }
  }
  return paths;
}
