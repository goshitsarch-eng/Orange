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
  using EditRequestCallback = std::function<void()>;
  using EditCommitCallback = std::function<void(int, PlaylistColumn, const std::string &)>;
  using DropUrlsCallback = std::function<void(const std::vector<std::string> &, int)>;
  using ReorderCallback = std::function<void(const std::vector<int> &, int)>;

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
  void SetEditRequestCallback(EditRequestCallback callback);
  void SetEditCommitCallback(EditCommitCallback callback);
  void SetDropUrlsCallback(DropUrlsCallback callback);
  void SetReorderCallback(ReorderCallback callback);
  int RowAtY(double y) const;
  PlaylistColumn last_clicked_column() const { return last_clicked_column_; }
  void SetLastClickedColumn(PlaylistColumn column) { last_clicked_column_ = column; }
  void StartInlineEdit(int row, PlaylistColumn column);
  int visible_count() const { return visible_count_; }

 private:
  void Clear();
  void RecordClickedColumn(GtkWidget *row, double x);
  void SetupRowDrag(GtkWidget *row, int index);
  gboolean OnDrop(const GValue *value, double y);

  GtkWidget *widget_ = nullptr;
  GtkWidget *grid_ = nullptr;
  std::string filter_;
  std::vector<int> selected_rows_;
  ActivateCallback activate_;
  SelectCallback select_;
  SortCallback sort_;
  MenuCallback menu_;
  EditRequestCallback edit_request_;
  EditCommitCallback edit_commit_;
  DropUrlsCallback drop_urls_;
  ReorderCallback reorder_;
  PlaylistColumn last_clicked_column_ = PlaylistColumn::Title;
  int visible_count_ = 0;
};

#endif
