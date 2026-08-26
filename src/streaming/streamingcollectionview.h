#ifndef STRAWBERRY_STREAMINGCOLLECTIONVIEW_H
#define STRAWBERRY_STREAMINGCOLLECTIONVIEW_H

#include "collection/collectiongrouping.h"
#include "core/song.h"
#include "streaming/streamingservices.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <vector>

class StreamingCollectionView {
 public:
  using ActivateCallback = std::function<void(const Song &)>;
  using RefreshCallback = std::function<void()>;
  using MenuCallback = std::function<void(const SongList &)>;
  using GroupingCallback = std::function<void(const CollectionGrouping::Grouping &)>;

  explicit StreamingCollectionView(const std::string &title);
  ~StreamingCollectionView();

  GtkWidget *widget() const { return widget_; }
  void SetSongs(const SongList &songs);
  void PushSongs(const SongList &songs);
  void PopBrowse();
  bool CanGoBack() const { return !stack_.empty(); }
  void SetStatus(const std::string &status);
  void SetFilter(const std::string &filter);
  void SetActivateCallback(ActivateCallback callback);
  void SetRefreshCallback(RefreshCallback callback);
  void SetMenuCallback(MenuCallback callback);
  void SetGroupingChangedCallback(GroupingCallback callback) { grouping_changed_ = std::move(callback); }
  void SetGrouping(const CollectionGrouping::Grouping &grouping);
  void ApplyGrouping(const CollectionGrouping::Grouping &grouping);
  const CollectionGrouping::Grouping &grouping() const { return grouping_; }
  const SongList &songs() const { return songs_; }
  SongList Visible() const;
  SongList SelectedSongs() const;

 private:
  struct Level {
    SongList songs;
    std::string status;
  };

  void Rebuild();
  void BuildGroupMenu();
  void SetupRowDrag(GtkWidget *row, const Song &song);
  void UpdateBack();
  void ActivateSong(const Song &song);
  gboolean OnKeyPressed(guint keyval);
  void ResetTypeAhead();

  GtkWidget *widget_ = nullptr;
  GtkWidget *back_ = nullptr;
  GtkWidget *group_button_ = nullptr;
  GtkWidget *filter_entry_ = nullptr;
  GtkWidget *status_label_ = nullptr;
  GtkWidget *list_ = nullptr;
  SongList songs_;
  std::vector<Level> stack_;
  std::string filter_;
  CollectionGrouping::Grouping grouping_;
  ActivateCallback activate_;
  RefreshCallback refresh_;
  MenuCallback menu_;
  GroupingCallback grouping_changed_;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
};

#endif
