#ifndef STRAWBERRY_PLAYLISTVIEW_H
#define STRAWBERRY_PLAYLISTVIEW_H

#include "playlist/playlist.h"
#include "playlist/playlistdelegates.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <vector>

class PlaylistView {
 public:
  using ActivateCallback = std::function<void(int)>;
  using SelectCallback = std::function<void(int, bool)>;
  using SortCallback = std::function<void(PlaylistColumn)>;
  using MenuCallback = std::function<void(double, double)>;

  PlaylistView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *grid() const { return grid_; }
  void SetFilterString(const std::string &filter);
  void SetSelectedRows(const std::vector<int> &rows);
  void Refresh(Playlist *playlist);
  void ScrollToRow(int row);
  void SetActivateCallback(ActivateCallback callback);
  void SetSelectCallback(SelectCallback callback);
  void SetSortCallback(SortCallback callback);
  void SetMenuCallback(MenuCallback callback);
  int visible_count() const { return visible_count_; }

 private:
  void Clear();

  GtkWidget *widget_ = nullptr;
  GtkWidget *grid_ = nullptr;
  std::string filter_;
  std::vector<int> selected_rows_;
  ActivateCallback activate_;
  SelectCallback select_;
  SortCallback sort_;
  MenuCallback menu_;
  int visible_count_ = 0;
};

#endif
