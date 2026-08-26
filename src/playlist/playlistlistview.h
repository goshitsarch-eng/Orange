#ifndef STRAWBERRY_PLAYLISTLISTVIEW_H
#define STRAWBERRY_PLAYLISTLISTVIEW_H

#include "playlist/playlistlistdrop.h"
#include "playlist/playlistlistlook.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <vector>

class PlaylistListView {
 public:
  using ActivateCallback = std::function<void(const std::string &)>;
  using MenuCallback = std::function<void(const std::string &)>;
  using DropCallback = std::function<void(const std::string &, const std::string &, bool)>;
  using FolderCallback = std::function<void(const std::string &)>;

  PlaylistListView();
  ~PlaylistListView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  void Refresh(const std::vector<PlaylistListDrop::Row> &rows, const std::string &current, const std::string &active = {},
               PlaylistListLook::Playback playback = PlaylistListLook::Playback::Stopped);
  void SelectName(const std::string &name);
  void SetActivateCallback(ActivateCallback callback);
  void SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }
  void SetDropCallback(DropCallback callback) { drop_ = std::move(callback); }
  void SetFolderToggleCallback(FolderCallback callback) { toggle_ = std::move(callback); }
  void SetDeleteCallback(ActivateCallback callback) { delete_ = std::move(callback); }
  std::string SelectedName() const;
  std::string SelectedFolderPath() const;
  bool SelectedIsFolder() const;

 private:
  void SetupRowDrop(GtkWidget *row, const PlaylistListDrop::Row &item);
  void SetupRowDrag(GtkWidget *row, const std::string &name);
  void StartDragHover(const std::string &name);
  void CancelDragHover();
  gboolean OnKeyPressed(guint keyval);
  void ResetTypeAhead();

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  ActivateCallback activate_;
  MenuCallback menu_;
  DropCallback drop_;
  FolderCallback toggle_;
  ActivateCallback delete_;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
  std::string current_;
  std::string hover_name_;
  guint hover_timeout_ = 0;
};

#endif
