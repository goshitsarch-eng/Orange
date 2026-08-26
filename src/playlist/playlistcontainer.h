#ifndef STRAWBERRY_PLAYLISTCONTAINER_H
#define STRAWBERRY_PLAYLISTCONTAINER_H

#include "playlist/dynamicplaylistcontrols.h"
#include "playlist/playlistsequence.h"
#include "playlist/playlisttabbar.h"
#include "playlist/playlistview.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class PlaylistManager;

class PlaylistContainer {
 public:
  using ActionCallback = std::function<void()>;

  PlaylistContainer();

  GtkWidget *widget() const { return widget_; }
  PlaylistView *view() { return view_.get(); }
  PlaylistTabBar *tab_bar() { return tab_bar_.get(); }
  DynamicPlaylistControls *dynamic_controls() { return dynamic_controls_.get(); }
  GtkWidget *summary() const { return summary_; }
  GtkWidget *repeat_button() const { return repeat_button_; }
  GtkWidget *shuffle_button() const { return shuffle_button_; }
  const std::string &filter_string() const { return filter_; }

  void SetFilterChangedCallback(const std::function<void(const std::string &)> &callback);
  void SetActionCallback(const char *name, ActionCallback callback);
  void SetRepeatChangedCallback(const std::function<void(PlaylistSequence::RepeatMode)> &callback);
  void SetShuffleChangedCallback(const std::function<void(PlaylistSequence::ShuffleMode)> &callback);
  void SetRepeatMode(PlaylistSequence::RepeatMode mode);
  void SetShuffleMode(PlaylistSequence::ShuffleMode mode);
  void SetSummary(const std::string &text);
  void ApplyLook();
  void FocusFilter();
  void SetFilterText(const std::string &text);
  void UpdateNoMatchesOverlay();
  void UpdateUndoRedoChrome(bool can_undo, bool can_redo);
  GtkWidget *filter_entry() const { return filter_entry_; }

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *toolbar_ = nullptr;
  GtkWidget *clear_button_ = nullptr;
  GtkWidget *undo_button_ = nullptr;
  GtkWidget *redo_button_ = nullptr;
  GtkWidget *summary_ = nullptr;
  GtkWidget *repeat_button_ = nullptr;
  GtkWidget *shuffle_button_ = nullptr;
  GtkWidget *filter_entry_ = nullptr;
  bool updating_filter_ = false;
  std::vector<GtkWidget *> repeat_items_;
  std::vector<GtkWidget *> shuffle_items_;
  bool updating_sequence_ = false;
  std::function<void(PlaylistSequence::RepeatMode)> repeat_changed_;
  std::function<void(PlaylistSequence::ShuffleMode)> shuffle_changed_;
  std::unique_ptr<PlaylistTabBar> tab_bar_;
  std::unique_ptr<PlaylistView> view_;
  std::unique_ptr<DynamicPlaylistControls> dynamic_controls_;
  std::string filter_;
  std::function<void(const std::string &)> filter_changed_;
};

#endif
