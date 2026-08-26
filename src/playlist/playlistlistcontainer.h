#ifndef STRAWBERRY_PLAYLISTLISTCONTAINER_H
#define STRAWBERRY_PLAYLISTLISTCONTAINER_H

#include "playlist/playlistlistlook.h"
#include "playlist/playlistlistmodel.h"
#include "playlist/playlistlistsortfiltermodel.h"
#include "playlist/playlistlistview.h"
#include "playlist/playlistmanager.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

class PlaylistListContainer {
 public:
  using NameCallback = std::function<void(const std::string &)>;
  using DropCallback = std::function<void(const std::string &, const std::string &, bool)>;

  PlaylistListContainer();

  GtkWidget *widget() const { return widget_; }
  PlaylistListView *view() { return view_.get(); }
  PlaylistListModel *model() { return &model_; }
  PlaylistListSortFilterModel *filter() { return &filter_; }
  void Reload(PlaylistManager *manager);
  void SetActivateCallback(const std::function<void(const std::string &)> &callback);
  void SetNewCallback(std::function<void()> callback) { new_ = std::move(callback); }
  void SetNewFolderCallback(std::function<void()> callback) { new_folder_ = std::move(callback); }
  void SetDeleteCallback(NameCallback callback) { delete_ = std::move(callback); }
  void SetDeleteFolderCallback(NameCallback callback) { delete_folder_ = std::move(callback); }
  void SetSaveCallback(NameCallback callback) { save_ = std::move(callback); }
  void SetCopyCallback(NameCallback callback) { copy_ = std::move(callback); }
  void SetMenuCallback(NameCallback callback) { menu_ = std::move(callback); }
  void SetDropCallback(DropCallback callback);
  std::string SelectedName() const;
  std::string SelectedFolderPath() const;
  bool SelectedIsFolder() const;
  void ApplyFilter();
  void AddExtraFolder(const std::string &path);
  void RemoveExtraFolder(const std::string &path);
  void RenameExtraFolder(const std::string &old_path, const std::string &new_path);
  void SetPlayback(PlaylistListLook::Playback playback);
  void SetActive(const std::string &name, int id);
  void SelectName(const std::string &name);
  PlaylistListLook::Playback playback() const { return playback_; }
  const std::string &active_name() const { return active_name_; }
  int active_id() const { return active_id_; }
  const std::vector<std::string> &extra_folders() const { return extra_folders_; }

 private:
  void Rebuild();
  void ToggleFolder(const std::string &path);
  void LoadExtraFolders();
  void SaveExtraFolders();

  GtkWidget *widget_ = nullptr;
  GtkWidget *search_ = nullptr;
  GtkWidget *favorites_toggle_ = nullptr;
  PlaylistListModel model_;
  PlaylistListSortFilterModel filter_;
  std::unique_ptr<PlaylistListView> view_;
  PlaylistManager *manager_ = nullptr;
  std::function<void()> new_;
  std::function<void()> new_folder_;
  NameCallback delete_;
  NameCallback delete_folder_;
  NameCallback save_;
  NameCallback copy_;
  NameCallback menu_;
  std::vector<std::string> extra_folders_;
  std::set<std::string> collapsed_;
  std::string active_name_;
  int active_id_ = -1;
  PlaylistListLook::Playback playback_ = PlaylistListLook::Playback::Stopped;
};

#endif
