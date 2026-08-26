#include "fileview/fileviewtree.h"

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
}

void FileViewTree::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void FileViewTree::AppendItem(GtkWidget *parent, FileViewTreeItem *item) {
  if (!item) {
    return;
  }
  if (!item->path.empty()) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(item->name.c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 6);
    gtk_widget_set_margin_bottom(label, 6);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data_full(G_OBJECT(row), "file-path", g_strdup(item->path.c_str()), g_free);
    gtk_list_box_append(GTK_LIST_BOX(parent), row);
  }
  for (const auto &child : item->children) {
    AppendItem(parent, child.get());
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
    AppendItem(list_, item.get());
  }
}
