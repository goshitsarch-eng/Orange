#ifndef STRAWBERRY_PLAYLISTLISTCONTAINER_H
#define STRAWBERRY_PLAYLISTLISTCONTAINER_H

#include "playlist/playlistlistmodel.h"
#include "playlist/playlistlistsortfiltermodel.h"
#include "playlist/playlistlistview.h"
#include "playlist/playlistmanager.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>

class PlaylistListContainer {
 public:
  using NameCallback = std::function<void(const std::string &)>;
  using DropCallback = std::function<void(const std::string &, const std::string &)>;

  PlaylistListContainer();

  GtkWidget *widget() const { return widget_; }
  PlaylistListView *view() { return view_.get(); }
  PlaylistListModel *model() { return &model_; }
  PlaylistListSortFilterModel *filter() { return &filter_; }
  void Reload(PlaylistManager *manager);
  void SetActivateCallback(const std::function<void(const std::string &)> &callback);
  void SetNewCallback(std::function<void()> callback) { new_ = std::move(callback); }
  void SetDeleteCallback(NameCallback callback) { delete_ = std::move(callback); }
  void SetSaveCallback(NameCallback callback) { save_ = std::move(callback); }
  void SetCopyCallback(NameCallback callback) { copy_ = std::move(callback); }
  void SetMenuCallback(NameCallback callback) { menu_ = std::move(callback); }
  void SetDropCallback(DropCallback callback);
  std::string SelectedName() const;
  void ApplyFilter();

 private:
  void Rebuild();

  GtkWidget *widget_ = nullptr;
  GtkWidget *search_ = nullptr;
  GtkWidget *favorites_toggle_ = nullptr;
  PlaylistListModel model_;
  PlaylistListSortFilterModel filter_;
  std::unique_ptr<PlaylistListView> view_;
  PlaylistManager *manager_ = nullptr;
  std::function<void()> new_;
  NameCallback delete_;
  NameCallback save_;
  NameCallback copy_;
  NameCallback menu_;
};

#endif
