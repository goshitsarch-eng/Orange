#ifndef STRAWBERRY_PLAYLISTVIEW_H
#define STRAWBERRY_PLAYLISTVIEW_H

#include "moodbar/moodbaritemdelegate.h"
#include "playlist/playlist.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistheader.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class PlaylistView {
 public:
  using ActivateCallback = std::function<void(int)>;
  using SelectCallback = std::function<void(int, bool)>;
  using SortCallback = std::function<void(PlaylistColumn, PlaylistSortOrder)>;
  using MenuCallback = std::function<void(double, double)>;
  using EditRequestCallback = std::function<void()>;
  using EditCommitCallback = std::function<void(int, PlaylistColumn, const std::string &)>;
  using DropUrlsCallback = std::function<void(const std::vector<std::string> &, int)>;
  using ReorderCallback = std::function<void(const std::vector<int> &, int)>;
  using RateCallback = std::function<void(int, float)>;
  using QueuePositionCallback = std::function<int(int)>;
  using DeleteCallback = std::function<void()>;
  using FocusFilterCallback = std::function<void()>;

  PlaylistView();
  ~PlaylistView();

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
  void SetRateCallback(RateCallback callback);
  void SetQueuePositionCallback(QueuePositionCallback callback);
  void SetDeleteCallback(DeleteCallback callback);
  void SetFocusFilterCallback(FocusFilterCallback callback);
  void FilterReturnPressed();
  void FocusAndMove(unsigned keyval);
  int RowAtY(double y) const;
  PlaylistColumn last_clicked_column() const { return last_clicked_column_; }
  void SetLastClickedColumn(PlaylistColumn column) { last_clicked_column_ = column; }
  double last_click_cell_x() const { return last_click_cell_x_; }
  double last_click_cell_width() const { return last_click_cell_width_; }
  void StartInlineEdit(int row, PlaylistColumn column);
  int visible_count() const { return visible_count_; }
  void SetPlaybackProgress(double progress);

 private:
  void Clear();
  void QueueDrawMoodbars();
  void RecordClickedColumn(GtkWidget *row, double x);
  void SetupRowDrag(GtkWidget *row, int index);
  gboolean OnDrop(const GValue *value, double y);

  GtkWidget *widget_ = nullptr;
  GtkWidget *grid_ = nullptr;
  std::unique_ptr<PlaylistHeader> header_;
  std::unique_ptr<MoodbarItemDelegate> moodbar_;
  Playlist *playlist_ = nullptr;
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
  RateCallback rate_;
  QueuePositionCallback queue_position_;
  DeleteCallback delete_;
  FocusFilterCallback focus_filter_;
  PlaylistColumn last_clicked_column_ = PlaylistColumn::Title;
  double last_click_cell_x_ = 0;
  double last_click_cell_width_ = 0;
  int visible_count_ = 0;
  double playback_progress_ = 0;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
  std::vector<std::string> visible_titles_;
  std::vector<int> visible_rows_;

  gboolean OnKeyPressed(guint keyval);
  void ResetTypeAhead();
};

#endif
