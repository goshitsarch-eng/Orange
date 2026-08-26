#include "playlist/playlistlistcontainer.h"

#include "constants/playlistsettings.h"
#include "core/settings.h"
#include "playlist/playlistfolders.h"
#include "translations/translations.h"

#include <algorithm>

PlaylistListContainer::PlaylistListContainer()
    : widget_(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)), filter_(&model_), view_(std::make_unique<PlaylistListView>()) {
  LoadExtraFolders();
  filter_.SetExtraFolders(extra_folders_);
  search_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search_), Translations::CStr("Filter playlists"));
  gtk_widget_set_margin_start(search_, 8);
  gtk_widget_set_margin_end(search_, 8);
  gtk_widget_set_margin_top(search_, 6);
  g_signal_connect(search_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     self->filter_.SetFilter(text ? text : "");
                     self->Rebuild();
                   }),
                   this);
  GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(bar, 8);
  gtk_widget_set_margin_end(bar, 8);
  gtk_widget_set_margin_bottom(bar, 4);
  GtkWidget *add = gtk_button_new_from_icon_name("list-add-symbolic");
  gtk_widget_add_css_class(add, "flat");
  gtk_widget_set_tooltip_text(add, Translations::CStr("New playlist"));
  GtkWidget *folder = gtk_button_new_from_icon_name("folder-new-symbolic");
  gtk_widget_add_css_class(folder, "flat");
  gtk_widget_set_tooltip_text(folder, Translations::CStr("New folder"));
  GtkWidget *remove = gtk_button_new_from_icon_name("list-remove-symbolic");
  gtk_widget_add_css_class(remove, "flat");
  gtk_widget_set_tooltip_text(remove, Translations::CStr("Delete playlist"));
  GtkWidget *save = gtk_button_new_from_icon_name("document-save-symbolic");
  gtk_widget_add_css_class(save, "flat");
  gtk_widget_set_tooltip_text(save, Translations::CStr("Save playlist"));
  GtkWidget *copy = gtk_button_new_from_icon_name("drive-harddisk-usb-symbolic");
  gtk_widget_add_css_class(copy, "flat");
  gtk_widget_set_tooltip_text(copy, Translations::CStr("Copy to device…"));
  favorites_toggle_ = gtk_toggle_button_new();
  gtk_button_set_icon_name(GTK_BUTTON(favorites_toggle_), "starred-symbolic");
  gtk_widget_add_css_class(favorites_toggle_, "flat");
  gtk_widget_set_tooltip_text(favorites_toggle_, Translations::CStr("Show favorite playlists only"));
  g_signal_connect(add, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     if (self->new_) {
                       self->new_();
                     }
                   }),
                   this);
  g_signal_connect(folder, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     if (self->new_folder_) {
                       self->new_folder_();
                     }
                   }),
                   this);
  g_signal_connect(remove, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     if (self->SelectedIsFolder()) {
                       if (self->delete_folder_) {
                         self->delete_folder_(self->SelectedFolderPath());
                       }
                       return;
                     }
                     if (self->delete_) {
                       self->delete_(self->SelectedName());
                     }
                   }),
                   this);
  g_signal_connect(save, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     if (self->save_) {
                       self->save_(self->SelectedName());
                     }
                   }),
                   this);
  g_signal_connect(copy, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     if (self->copy_) {
                       self->copy_(self->SelectedName());
                     }
                   }),
                   this);
  g_signal_connect(favorites_toggle_, "toggled", G_CALLBACK(+[](GtkToggleButton *button, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     self->filter_.SetFavoritesOnly(gtk_toggle_button_get_active(button) == TRUE);
                     self->Rebuild();
                   }),
                   this);
  gtk_box_append(GTK_BOX(bar), add);
  gtk_box_append(GTK_BOX(bar), folder);
  gtk_box_append(GTK_BOX(bar), remove);
  gtk_box_append(GTK_BOX(bar), save);
  gtk_box_append(GTK_BOX(bar), copy);
  gtk_box_append(GTK_BOX(bar), favorites_toggle_);
  gtk_widget_set_vexpand(view_->widget(), TRUE);
  gtk_box_append(GTK_BOX(widget_), search_);
  gtk_box_append(GTK_BOX(widget_), bar);
  gtk_box_append(GTK_BOX(widget_), view_->widget());
  view_->SetMenuCallback([this](const std::string &name) {
    if (menu_) {
      menu_(name);
    }
  });
  view_->SetFolderToggleCallback([this](const std::string &path) { ToggleFolder(path); });
  view_->SetDeleteCallback([this](const std::string &name) {
    if (SelectedIsFolder()) {
      if (delete_folder_) {
        delete_folder_(SelectedFolderPath());
      }
      return;
    }
    if (delete_) {
      delete_(name);
    }
  });
}

void PlaylistListContainer::Reload(PlaylistManager *manager) {
  manager_ = manager;
  model_.Reload(manager);
  Rebuild();
}

void PlaylistListContainer::Rebuild() {
  std::string current;
  if (manager_ && manager_->current()) {
    current = manager_->current()->name();
  }
  if (active_name_.empty() && manager_ && manager_->active()) {
    active_name_ = manager_->active()->name();
    active_id_ = manager_->active()->id();
  }
  filter_.SetExtraFolders(extra_folders_);
  filter_.SetCollapsed(collapsed_);
  view_->Refresh(filter_.VisibleRows(), current, active_name_, playback_);
}

void PlaylistListContainer::SetPlayback(PlaylistListLook::Playback playback) {
  playback_ = playback;
  Rebuild();
}

void PlaylistListContainer::SetActive(const std::string &name, int id) {
  active_name_ = name;
  active_id_ = id;
  Rebuild();
}

void PlaylistListContainer::SelectName(const std::string &name) {
  if (view_) {
    view_->SelectName(name);
  }
}

void PlaylistListContainer::SetActivateCallback(const std::function<void(const std::string &)> &callback) {
  view_->SetActivateCallback(callback);
}

void PlaylistListContainer::SetDropCallback(DropCallback callback) { view_->SetDropCallback(std::move(callback)); }

std::string PlaylistListContainer::SelectedName() const { return view_->SelectedName(); }

std::string PlaylistListContainer::SelectedFolderPath() const { return view_->SelectedFolderPath(); }

bool PlaylistListContainer::SelectedIsFolder() const { return view_->SelectedIsFolder(); }

void PlaylistListContainer::ApplyFilter() { Rebuild(); }

void PlaylistListContainer::ToggleFolder(const std::string &path) {
  if (path.empty()) {
    return;
  }
  if (collapsed_.erase(path) == 0) {
    collapsed_.insert(path);
  }
  Rebuild();
}

void PlaylistListContainer::AddExtraFolder(const std::string &path) {
  if (path.empty()) {
    return;
  }
  if (std::find(extra_folders_.begin(), extra_folders_.end(), path) == extra_folders_.end()) {
    extra_folders_.push_back(path);
    SaveExtraFolders();
  }
  Rebuild();
}

void PlaylistListContainer::RemoveExtraFolder(const std::string &path) {
  extra_folders_.erase(std::remove_if(extra_folders_.begin(), extra_folders_.end(),
                                      [&](const std::string &folder) { return PlaylistFolders::IsUnder(folder, path); }),
                       extra_folders_.end());
  collapsed_.erase(path);
  SaveExtraFolders();
  Rebuild();
}

void PlaylistListContainer::RenameExtraFolder(const std::string &old_path, const std::string &new_path) {
  for (std::string &folder : extra_folders_) {
    folder = PlaylistFolders::RenamePrefix(folder, old_path, new_path);
  }
  std::set<std::string> collapsed;
  for (const std::string &folder : collapsed_) {
    collapsed.insert(PlaylistFolders::RenamePrefix(folder, old_path, new_path));
  }
  collapsed_ = std::move(collapsed);
  SaveExtraFolders();
  Rebuild();
}

void PlaylistListContainer::LoadExtraFolders() {
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  extra_folders_ = PlaylistFolders::ParseFolderList(settings.Value(PlaylistSettings::kUiFolders));
}

void PlaylistListContainer::SaveExtraFolders() {
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  settings.SetValue(PlaylistSettings::kUiFolders, PlaylistFolders::JoinFolderList(extra_folders_));
  settings.Sync();
}
