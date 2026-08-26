#include "fileview/fileview.h"

#include "core/settings.h"
#include "fileview/fileviewsettings.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"

#include <adwaita.h>

#include <string>

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
  gtk_widget_set_tooltip_text(back_, Translations::CStr("Back"));
  forward_ = gtk_button_new_from_icon_name("go-next-symbolic");
  gtk_widget_set_tooltip_text(forward_, Translations::CStr("Forward"));
  GtkWidget *up = gtk_button_new_from_icon_name("go-up-symbolic");
  gtk_widget_set_tooltip_text(up, Translations::CStr("Up"));
  GtkWidget *home = gtk_button_new_from_icon_name("go-home-symbolic");
  gtk_widget_set_tooltip_text(home, Translations::CStr("Home"));
  path_entry_ = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(path_entry_), path_.c_str());
  gtk_widget_set_hexpand(path_entry_, TRUE);
  hidden_btn_ = gtk_toggle_button_new_with_label(Translations::CStr("Hidden"));
  gtk_widget_set_tooltip_text(hidden_btn_, Translations::CStr("Show hidden files"));
  all_files_btn_ = gtk_toggle_button_new_with_label(Translations::CStr("All files"));
  gtk_widget_set_tooltip_text(all_files_btn_, Translations::CStr("Show files that are not audio or playlists"));
  gtk_box_append(GTK_BOX(toolbar), back_);
  gtk_box_append(GTK_BOX(toolbar), forward_);
  gtk_box_append(GTK_BOX(toolbar), up);
  gtk_box_append(GTK_BOX(toolbar), home);
  gtk_box_append(GTK_BOX(toolbar), path_entry_);
  gtk_box_append(GTK_BOX(toolbar), hidden_btn_);
  gtk_box_append(GTK_BOX(toolbar), all_files_btn_);
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
  tree_->SetMenuCallback([this](const std::string &path) {
    if (!path.empty()) {
      ShowMenu({path});
    }
  });
  list_->SetNavigateCallback([this](FileViewKeyboard::Action action) {
    action = FileViewKeyboard::ResolveHistoryBack(action, history_.CanBack());
    switch (action) {
      case FileViewKeyboard::Action::UpDir:
        FileUp();
        break;
      case FileViewKeyboard::Action::HistoryBack:
        FileBack();
        break;
      case FileViewKeyboard::Action::HistoryForward:
        FileForward();
        break;
      case FileViewKeyboard::Action::Home:
        FileHome();
        break;
      default:
        break;
    }
  });
  Settings settings;
  settings.BeginGroup(FileViewSettings::kSettingsGroup);
  const bool show_hidden = settings.BoolValue(FileViewSettings::kShowHidden, FileViewSettings::kDefaultShowHidden);
  const bool show_all = settings.BoolValue(FileViewSettings::kShowAllFiles, FileViewSettings::kDefaultShowAllFiles);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hidden_btn_), show_hidden);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(all_files_btn_), show_all);
  model_.SetShowHidden(show_hidden);
  model_.SetShowAllFiles(show_all);
  g_signal_connect(hidden_btn_, "toggled", G_CALLBACK(+[](GtkToggleButton *button, gpointer data) {
                     static_cast<FileView *>(data)->SetShowHidden(gtk_toggle_button_get_active(button));
                   }),
                   this);
  g_signal_connect(all_files_btn_, "toggled", G_CALLBACK(+[](GtkToggleButton *button, gpointer data) {
                     static_cast<FileView *>(data)->SetShowAllFiles(gtk_toggle_button_get_active(button));
                   }),
                   this);
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

void FileView::SetShowHidden(bool show_hidden) {
  model_.SetShowHidden(show_hidden);
  PersistSettings();
  Reload();
}

void FileView::SetShowAllFiles(bool show_all) {
  model_.SetShowAllFiles(show_all);
  PersistSettings();
  Reload();
}

void FileView::PersistSettings() {
  Settings settings;
  settings.BeginGroup(FileViewSettings::kSettingsGroup);
  settings.SetBoolValue(FileViewSettings::kShowHidden, model_.show_hidden());
  settings.SetBoolValue(FileViewSettings::kShowAllFiles, model_.show_all_files());
  settings.Sync();
}

void FileView::SetAddToPlaylistCallback(PathsCallback callback) { add_to_playlist_ = std::move(callback); }

void FileView::SetReplacePlaylistCallback(PathsCallback callback) { replace_playlist_ = std::move(callback); }

void FileView::SetOpenInNewCallback(PathsCallback callback) { open_in_new_ = std::move(callback); }

void FileView::SetCopyToCollectionCallback(PathsCallback callback) { copy_to_collection_ = std::move(callback); }

void FileView::SetMoveToCollectionCallback(PathsCallback callback) { move_to_collection_ = std::move(callback); }

void FileView::SetCopyToDeviceCallback(PathsCallback callback) { copy_to_device_ = std::move(callback); }

void FileView::SetEditTagsCallback(PathsCallback callback) { edit_tags_ = std::move(callback); }

void FileView::SetDeleteCallback(PathsCallback callback) { delete_ = std::move(callback); }

void FileView::SetShowInBrowserCallback(PathsCallback callback) { show_in_browser_ = std::move(callback); }

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
  for (const FileViewMenu::Item &item : FileViewMenu::Items()) {
    g_menu_append(menu, Translations::CStr(item.label), (std::string("fileview.") + item.id).c_str());
  }
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, list_->widget());
  auto *state = new std::pair<FileView *, std::vector<std::string>>(this, paths);
  g_object_set_data_full(G_OBJECT(popover), "fileview-menu", state, [](gpointer p) {
    delete static_cast<std::pair<FileView *, std::vector<std::string>> *>(p);
  });
  GSimpleActionGroup *group = g_simple_action_group_new();
  for (const FileViewMenu::Item &item : FileViewMenu::Items()) {
    GSimpleAction *action = g_simple_action_new(item.id, nullptr);
    g_object_set_data(G_OBJECT(action), "popover", popover);
    g_signal_connect(action, "activate", G_CALLBACK((+[](GSimpleAction *act, GVariant *, gpointer data) {
                       GtkWidget *pop = GTK_WIDGET(g_object_get_data(G_OBJECT(act), "popover"));
                       using MenuState = std::pair<FileView *, std::vector<std::string>>;
                       auto *pair = static_cast<MenuState *>(g_object_get_data(G_OBJECT(pop), "fileview-menu"));
                       auto *self = static_cast<FileView *>(data);
                       if (!pair) {
                         return;
                       }
                       const FileViewMenu::Action action = FileViewMenu::FromId(g_action_get_name(G_ACTION(act)));
                       switch (action) {
                         case FileViewMenu::Action::Append:
                           if (self->add_to_playlist_) {
                             self->add_to_playlist_(FileViewMenu::ExpandPaths(pair->second));
                           }
                           break;
                         case FileViewMenu::Action::Replace:
                           if (self->replace_playlist_) {
                             self->replace_playlist_(FileViewMenu::ExpandPaths(pair->second));
                           }
                           break;
                         case FileViewMenu::Action::New:
                           if (self->open_in_new_) {
                             self->open_in_new_(FileViewMenu::ExpandPaths(pair->second));
                           }
                           break;
                         case FileViewMenu::Action::Copy:
                           if (self->copy_to_collection_) {
                             self->copy_to_collection_(pair->second);
                           }
                           break;
                         case FileViewMenu::Action::Move:
                           if (self->move_to_collection_) {
                             self->move_to_collection_(pair->second);
                           }
                           break;
                         case FileViewMenu::Action::Device:
                           if (self->copy_to_device_) {
                             self->copy_to_device_(FileViewMenu::ExpandPaths(pair->second));
                           }
                           break;
                         case FileViewMenu::Action::EditTags:
                           if (self->edit_tags_) {
                             self->edit_tags_(FileViewMenu::ExpandPaths(pair->second));
                           }
                           break;
                         case FileViewMenu::Action::Delete:
                           if (self->delete_) {
                             self->delete_(FileViewMenu::ExpandPaths(pair->second));
                           }
                           break;
                         case FileViewMenu::Action::Browse:
                           if (self->show_in_browser_) {
                             self->show_in_browser_(pair->second);
                           }
                           break;
                       }
                     })),
                     this);
    g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
  }
  gtk_widget_insert_action_group(list_->widget(), "fileview", G_ACTION_GROUP(group));
  gtk_popover_popup(GTK_POPOVER(popover));
}
