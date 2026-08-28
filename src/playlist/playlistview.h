#ifndef STRAWBERRY_PLAYLISTVIEW_H
#define STRAWBERRY_PLAYLISTVIEW_H

#include "playlist/playlistselection.h"
#include "moodbar/moodbaritemdelegate.h"
#include "playlist/playlist.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistdropindicator.h"
#include "playlist/playlistheader.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class PlaylistView {
 public:
  using ActivateCallback = std::function<void(int)>;
  using SelectCallback = std::function<void(int, PlaylistSelection::Mode)>;
  using SortCallback = std::function<void(PlaylistColumn, PlaylistSortOrder)>;
  using MenuCallback = std::function<void(double, double)>;
  using EditRequestCallback = std::function<void()>;
  using EditCommitCallback = std::function<void(int, PlaylistColumn, const std::string &)>;
  using DropUrlsCallback = std::function<void(const std::vector<std::string> &, int)>;
  using ReorderCallback = std::function<void(const std::vector<int> &, int)>;
  using CrossDropCallback = std::function<void(int, const std::vector<int> &, int)>;
  using RateCallback = std::function<void(const std::vector<int> &, float)>;
  using QueuePositionCallback = std::function<int(int)>;
  using DeleteCallback = std::function<void()>;
  using FocusFilterCallback = std::function<void(unsigned keyval, const char *utf8)>;
  using PlayPauseCallback = std::function<void()>;
  using SeekCallback = std::function<void()>;

  PlaylistView();
  ~PlaylistView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *grid() const { return grid_; }
  void SetFilterString(const std::string &filter);
  void SetSelectedRows(const std::vector<int> &rows);
  // Updates the selection styling of the rows already on screen, without rebuilding them.
  void ApplySelection();
  void Refresh(Playlist *playlist);
  void ScrollToRow(int row, bool center = false);
  void JumpToCurrentlyPlayingTrack();
  void JumpToLastPlayedTrack();
  void HandleRatingHover(GtkWidget *row, double x);
  void ClearRatingHover();
  void MaybeScrollToRow(int row, Playlist::AutoScroll mode);
  void InhibitAutoscroll();
  void SetPaused(bool paused);
  void SetActivateCallback(ActivateCallback callback);
  void SetSelectCallback(SelectCallback callback);
  void SetSortCallback(SortCallback callback);
  void SetMenuCallback(MenuCallback callback);
  void SetEditRequestCallback(EditRequestCallback callback);
  void SetEditCommitCallback(EditCommitCallback callback);
  void SetDropUrlsCallback(DropUrlsCallback callback);
  void SetReorderCallback(ReorderCallback callback);
  void SetCrossDropCallback(CrossDropCallback callback);
  void SetRateCallback(RateCallback callback);
  void SetQueuePositionCallback(QueuePositionCallback callback);
  void SetDeleteCallback(DeleteCallback callback);
  void SetFocusFilterCallback(FocusFilterCallback callback);
  void SetPlayPauseCallback(PlayPauseCallback callback);
  void SetSeekBackwardCallback(SeekCallback callback);
  void SetSeekForwardCallback(SeekCallback callback);
  void FilterReturnPressed();
  void FocusAndMove(unsigned keyval);
  int RowAtY(double y) const;
  int RowAtY(double y, GtkWidget *relative) const;
  void RememberClickAt(double x, double y);
  PlaylistColumn last_clicked_column() const { return last_clicked_column_; }
  void SetLastClickedColumn(PlaylistColumn column) { last_clicked_column_ = column; }
  int last_clicked_row() const { return last_clicked_row_; }
  double last_click_cell_x() const { return last_click_cell_x_; }
  double last_click_cell_width() const { return last_click_cell_width_; }
  void StartInlineEdit(int row, PlaylistColumn column);
  int visible_count() const { return visible_count_; }
  void UpdateNoMatchesOverlay();
  void SetPlaybackProgress(double progress);
  void SetGlowing(bool glowing);
  void ReloadSettings();
  void SetBackground(const std::string &css, const std::string &key);

 private:
  void Clear();
  void QueueDrawMoodbars();
  void RecordClickedColumn(GtkWidget *row, double x);
  void UpdateRatingHoverLabels();
  void SetupRowDrag(GtkWidget *row, int index);
  gboolean OnDrop(const GValue *value, double y);
  void UpdateDropIndicator(double y);
  void ClearDropIndicator();
  void ApplyColumnWidths();
  int ViewportWidth() const;
  static int ColumnWidthFrom(const std::vector<PlaylistColumn> &columns, const std::vector<int> &widths, PlaylistColumn column);
  void ReloadLookCss();
  void StartGlowTimer();
  void StopGlowTimer();
  gboolean OnGlowTick();
  void StopBackgroundFade();
  gboolean OnBackgroundFadeTick();
  void ApplyBackgroundCss();
  void OnShown();
  void OnHidden();

  GtkWidget *widget_ = nullptr;
  // The scrolled window inside widget_, which is an overlay so the empty/no-matches status page can be
  // positioned against the viewport rather than against the (often wider) scrolled content.
  GtkWidget *scroller_ = nullptr;
  GtkWidget *grid_ = nullptr;
  GtkWidget *overlay_ = nullptr;
  GtkWidget *root_overlay_ = nullptr;
  GtkWidget *bg_overlay_ = nullptr;
  GtkWidget *current_bg_ = nullptr;
  GtkWidget *previous_bg_ = nullptr;
  GtkWidget *drop_overlay_ = nullptr;
  GtkWidget *no_matches_ = nullptr;
  PlaylistDropIndicator::State drop_state_;
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
  CrossDropCallback cross_drop_;
  RateCallback rate_;
  QueuePositionCallback queue_position_;
  DeleteCallback delete_;
  FocusFilterCallback focus_filter_;
  PlayPauseCallback play_pause_;
  SeekCallback seek_backward_;
  SeekCallback seek_forward_;
  PlaylistColumn last_clicked_column_ = PlaylistColumn::Count;
  int last_clicked_row_ = -1;
  int hover_rating_row_ = -1;
  float hover_rating_ = -1.0f;
  double last_click_cell_x_ = 0;
  double last_click_cell_width_ = 0;
  int visible_count_ = 0;
  double playback_progress_ = 0;
  int glow_step_ = 0;
  bool glowing_ = false;
  bool paused_ = false;
  guint glow_timeout_ = 0;
  // Cached look settings.
  // ReloadLookCss() runs on every glow animation tick, so it must not re-read the settings file each time.
  void ReloadLookSettings();
  bool look_alternating_ = true;
  bool look_glow_ = true;
  bool look_bars_ = true;
  bool look_settings_loaded_ = false;
  bool inhibit_autoscroll_ = false;
  guint inhibit_timeout_ = 0;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
  guint background_fade_id_ = 0;
  int background_fade_elapsed_ms_ = 0;
  std::string background_css_;
  std::string background_key_;
  std::string previous_background_css_;
  std::vector<std::string> visible_titles_;
  std::vector<int> visible_rows_;

  gboolean OnKeyPressed(guint keyval, GdkModifierType state);
  void ResetTypeAhead();
  void CopyCurrentToClipboard();
  bool IsRowVisible(int row) const;
};

#endif
