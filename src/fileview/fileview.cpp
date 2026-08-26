#include "fileview/fileview.h"

#include "utilities/fileutils.h"

#include <adwaita.h>

FileView::FileView() {
  const char *music = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);
  home_ = music && *music ? music : (g_get_home_dir() ? g_get_home_dir() : ".");
  path_ = home_;
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(toolbar, 8);
  gtk_widget_set_margin_end(toolbar, 8);
  gtk_widget_set_margin_top(toolbar, 6);
  gtk_widget_set_margin_bottom(toolbar, 4);
  back_ = gtk_button_new_from_icon_name("go-previous-symbolic");
  gtk_widget_set_tooltip_text(back_, "Back");
  forward_ = gtk_button_new_from_icon_name("go-next-symbolic");
  gtk_widget_set_tooltip_text(forward_, "Forward");
  GtkWidget *up = gtk_button_new_from_icon_name("go-up-symbolic");
  gtk_widget_set_tooltip_text(up, "Up");
  GtkWidget *home = gtk_button_new_from_icon_name("go-home-symbolic");
  gtk_widget_set_tooltip_text(home, "Home");
  path_entry_ = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(path_entry_), path_.c_str());
  gtk_widget_set_hexpand(path_entry_, TRUE);
  gtk_box_append(GTK_BOX(toolbar), back_);
  gtk_box_append(GTK_BOX(toolbar), forward_);
  gtk_box_append(GTK_BOX(toolbar), up);
  gtk_box_append(GTK_BOX(toolbar), home);
  gtk_box_append(GTK_BOX(toolbar), path_entry_);
  gtk_box_append(GTK_BOX(widget_), toolbar);

  tree_ = std::make_unique<FileViewTree>();
  list_ = std::make_unique<FileViewList>();
  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_vexpand(paned, TRUE);
  gtk_paned_set_start_child(GTK_PANED(paned), tree_->widget());
  gtk_paned_set_end_child(GTK_PANED(paned), list_->widget());
  gtk_paned_set_resize_start_child(GTK_PANED(paned), FALSE);
  gtk_box_append(GTK_BOX(widget_), paned);

  g_signal_connect(back_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<FileView *>(data)->FileBack(); }), this);
  g_signal_connect(forward_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<FileView *>(data)->FileForward(); }), this);
  g_signal_connect(up, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<FileView *>(data)->FileUp(); }), this);
  g_signal_connect(home, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<FileView *>(data)->FileHome(); }), this);
  g_signal_connect(path_entry_, "activate", G_CALLBACK(+[](GtkEntry *entry, gpointer data) {
                     static_cast<FileView *>(data)->SetPath(gtk_editable_get_text(GTK_EDITABLE(entry)));
                   }),
                   this);
  tree_->SetActivateCallback([this](const std::string &path) { SetPath(path); });
  list_->SetActivateCallback([this](const std::string &path) { Activate(path); });
  list_->SetMenuCallback([this](const std::vector<std::string> &paths) { ShowMenu(paths); });
  model_.SetRootPaths({home_});
  history_.Push(path_);
  Reload();
}

void FileView::SetPath(const std::string &path, bool record) {
  if (!FileUtils::IsDirectory(path)) {
    return;
  }
  path_ = path;
  if (record) {
    history_.Push(path_);
  }
  Reload();
}

void FileView::FileUp() { SetPath(FileUtils::DirName(path_)); }

void FileView::FileHome() { SetPath(home_); }

void FileView::FileBack() {
  if (history_.CanBack()) {
    SetPath(history_.Back(), false);
  }
}

void FileView::FileForward() {
  if (history_.CanForward()) {
    SetPath(history_.Forward(), false);
  }
}

void FileView::UpdateNavButtons() {
  if (back_) {
    gtk_widget_set_sensitive(back_, history_.CanBack());
  }
  if (forward_) {
    gtk_widget_set_sensitive(forward_, history_.CanForward());
  }
}

void FileView::Reload() {
  if (path_entry_) {
    gtk_editable_set_text(GTK_EDITABLE(path_entry_), path_.c_str());
  }
  UpdateNavButtons();
  model_.SetRootPaths({FileUtils::DirName(path_).empty() ? path_ : FileUtils::DirName(path_)});
  if (FileViewTreeItem *root = model_.root()) {
    for (const auto &child : root->children) {
      model_.LazyLoad(child.get());
    }
  }
  tree_->Reload(&model_);
  list_->Reload(model_.FilesIn(path_));
}

void FileView::SetAddToPlaylistCallback(PathsCallback callback) { add_to_playlist_ = std::move(callback); }

void FileView::SetCopyToCollectionCallback(PathsCallback callback) { copy_to_collection_ = std::move(callback); }

void FileView::SetMoveToCollectionCallback(PathsCallback callback) { move_to_collection_ = std::move(callback); }

void FileView::SetCopyToDeviceCallback(PathsCallback callback) { copy_to_device_ = std::move(callback); }

void FileView::SetEditTagsCallback(PathsCallback callback) { edit_tags_ = std::move(callback); }

void FileView::SetDeleteCallback(PathsCallback callback) { delete_ = std::move(callback); }

void FileView::Activate(const std::string &path) {
  if (FileUtils::IsDirectory(path)) {
    SetPath(path);
    return;
  }
  if (add_to_playlist_) {
    add_to_playlist_({path});
  }
}

void FileView::ShowMenu(const std::vector<std::string> &paths) {
  GMenu *menu = g_menu_new();
  g_menu_append(menu, "Add to playlist", "fileview.add");
  g_menu_append(menu, "Copy to collection", "fileview.copy-collection");
  g_menu_append(menu, "Move to collection", "fileview.move-collection");
  g_menu_append(menu, "Copy to device", "fileview.copy-device");
  g_menu_append(menu, "Edit track information…", "fileview.edit-tags");
  g_menu_append(menu, "Delete", "fileview.delete");
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, list_->widget());
  auto *state = new std::pair<FileView *, std::vector<std::string>>(this, paths);
  g_object_set_data_full(G_OBJECT(popover), "fileview-menu", state, [](gpointer p) {
    delete static_cast<std::pair<FileView *, std::vector<std::string>> *>(p);
  });
  GSimpleActionGroup *group = g_simple_action_group_new();
  const char *names[] = {"add", "copy-collection", "move-collection", "copy-device", "edit-tags", "delete", nullptr};
  for (int i = 0; names[i]; ++i) {
    GSimpleAction *action = g_simple_action_new(names[i], nullptr);
    g_object_set_data(G_OBJECT(action), "popover", popover);
    g_signal_connect(action, "activate", G_CALLBACK((+[](GSimpleAction *act, GVariant *, gpointer data) {
                       GtkWidget *pop = GTK_WIDGET(g_object_get_data(G_OBJECT(act), "popover"));
                       using MenuState = std::pair<FileView *, std::vector<std::string>>;
                       auto *pair = static_cast<MenuState *>(g_object_get_data(G_OBJECT(pop), "fileview-menu"));
                       auto *self = static_cast<FileView *>(data);
                       if (!pair) {
                         return;
                       }
                       const char *n = g_action_get_name(G_ACTION(act));
                       if (g_strcmp0(n, "add") == 0 && self->add_to_playlist_) {
                         self->add_to_playlist_(pair->second);
                       } else if (g_strcmp0(n, "copy-collection") == 0 && self->copy_to_collection_) {
                         self->copy_to_collection_(pair->second);
                       } else if (g_strcmp0(n, "move-collection") == 0 && self->move_to_collection_) {
                         self->move_to_collection_(pair->second);
                       } else if (g_strcmp0(n, "copy-device") == 0 && self->copy_to_device_) {
                         self->copy_to_device_(pair->second);
                       } else if (g_strcmp0(n, "edit-tags") == 0 && self->edit_tags_) {
                         self->edit_tags_(pair->second);
                       } else if (g_strcmp0(n, "delete") == 0 && self->delete_) {
                         self->delete_(pair->second);
                       }
                     })),
                     this);
    g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
  }
  gtk_widget_insert_action_group(list_->widget(), "fileview", G_ACTION_GROUP(group));
  gtk_popover_popup(GTK_POPOVER(popover));
}
