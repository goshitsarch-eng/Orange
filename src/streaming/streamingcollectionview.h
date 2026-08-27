#ifndef STRAWBERRY_STREAMINGCOLLECTIONVIEW_H
#define STRAWBERRY_STREAMINGCOLLECTIONVIEW_H

#include "collection/collectionfocus.h"
#include "collection/collectiongrouping.h"
#include "collection/collectionitem.h"
#include "collection/collectionmodel.h"
#include "core/song.h"
#include "streaming/streamingservices.h"

#include <gtk/gtk.h>

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class StreamingCollectionView {
 public:
  using ActivateCallback = std::function<void(const Song &)>;
  using EnqueueCallback = std::function<void(const SongList &)>;
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
  void SetEnqueueCallback(EnqueueCallback callback);
  void SetRefreshCallback(RefreshCallback callback);
  void SetMenuCallback(MenuCallback callback);
  void HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state);
  void FocusFilter();
  void FocusListAndMove(unsigned keyval);
  void SetGroupingChangedCallback(GroupingCallback callback) { grouping_changed_ = std::move(callback); }
  void SetService(StreamingService *service);
  void SetGrouping(const CollectionGrouping::Grouping &grouping);
  void ApplyGrouping(const CollectionGrouping::Grouping &grouping);
  const CollectionGrouping::Grouping &grouping() const { return grouping_; }
  bool pretty_covers() const { return pretty_covers_; }
  const SongList &songs() const { return songs_; }
  SongList Visible() const;
  SongList SelectedSongs() const;
  std::string SelectedSearchQuery() const;

 private:
  struct Level {
    SongList songs;
    std::string status;
  };

  void Rebuild(bool preserve_focus = true);
  void SaveFocus();
  void RestoreFocus();
  void SelectFocusItem();
  const CollectionItem *SelectedItem() const;
  void AppendItem(const CollectionItem *item, int depth, bool filter_active);
  void ToggleExpanded(const CollectionItem *item);
  void BuildGroupMenu();
  void SetupRowDrag(GtkWidget *row, const Song &song);
  void LoadCover(GtkWidget *image, const Song &song);
  void PersistPrettyCovers();
  void UpdateBack();
  void ActivateSong(const Song &song);
  gboolean OnKeyPressed(guint keyval);
  void ResetTypeAhead();

  StreamingService *service_ = nullptr;
  GtkWidget *widget_ = nullptr;
  GtkWidget *back_ = nullptr;
  GtkWidget *pretty_covers_btn_ = nullptr;
  GtkWidget *group_button_ = nullptr;
  GtkWidget *filter_entry_ = nullptr;
  GtkWidget *status_label_ = nullptr;
  GtkWidget *list_ = nullptr;
  SongList songs_;
  CollectionModel model_;
  std::set<std::string> expanded_;
  CollectionFocus::State focus_;
  std::vector<Level> stack_;
  std::string filter_;
  CollectionGrouping::Grouping grouping_;
  ActivateCallback activate_;
  EnqueueCallback enqueue_;
  RefreshCallback refresh_;
  MenuCallback menu_;
  GroupingCallback grouping_changed_;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
  bool pretty_covers_ = true;
  int cover_gen_ = 0;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  std::map<std::string, std::string> cover_cache_;
};

#endif
