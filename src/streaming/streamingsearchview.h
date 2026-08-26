#ifndef STRAWBERRY_STREAMINGSEARCHVIEW_H
#define STRAWBERRY_STREAMINGSEARCHVIEW_H

#include "collection/collectiongrouping.h"
#include "collection/collectionitem.h"
#include "collection/collectionmodel.h"
#include "streaming/streamingprogress.h"
#include "streaming/streamingsearchgroup.h"
#include "streaming/streamingsearchmodel.h"
#include "streaming/streamingsearchsortmodel.h"
#include "streaming/streamingservices.h"

#include <gtk/gtk.h>

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>

class StreamingSearchView {
 public:
  using ActivateCallback = std::function<void(const Song &)>;
  using EnqueueCallback = std::function<void(const SongList &)>;
  using MenuCallback = std::function<void(const SongList &)>;
  using ConfigureCallback = std::function<void()>;

  explicit StreamingSearchView(StreamingService *service);
  ~StreamingSearchView();

  GtkWidget *widget() const { return widget_; }
  void Search(const std::string &query);
  void SetActivateCallback(ActivateCallback callback);
  void SetEnqueueCallback(EnqueueCallback callback);
  void SetMenuCallback(MenuCallback callback);
  void HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state);
  void FocusSearch();
  void FocusResultsAndMove(unsigned keyval);
  void SetConfigureCallback(ConfigureCallback callback);
  void SearchForThis(const std::string &query = {});
  std::string SelectedSearchQuery() const;
  StreamingSearchModel *model() { return &model_; }
  const SongList &results() const { return model_.songs(); }
  SongList SelectedSongs() const;

 private:
  void Rebuild();
  void AppendItem(const CollectionItem *item, int depth, bool filter_active);
  void ToggleExpanded(const CollectionItem *item);
  void SetupRowDrag(GtkWidget *row, const Song &song);
  void LoadCover(GtkWidget *image, const Song &song);
  void PersistPrettyCovers();
  void PersistGrouping();
  void BuildGroupMenu();
  void ApplyGrouping(const CollectionGrouping::Grouping &grouping);
  void HideProgress();
  void HideProgressUnlessError();
  void ShowError(const std::string &status);
  void ApplyStatus(const std::string &text);
  void ApplyProgress(int value, int maximum);
  void ScheduleSearch(const std::string &query, bool immediate);
  void CancelPendingSearch();
  StreamingService::SearchType CurrentType() const;
  gboolean OnKeyPressed(guint keyval);
  void ResetTypeAhead();

  StreamingService *service_ = nullptr;
  StreamingSearchModel model_;
  StreamingSearchSortModel sort_model_{&model_};
  CollectionModel tree_model_;
  std::set<std::string> expanded_;
  ActivateCallback activate_;
  EnqueueCallback enqueue_;
  MenuCallback menu_;
  ConfigureCallback configure_;
  GtkWidget *widget_ = nullptr;
  GtkWidget *search_entry_ = nullptr;
  GtkWidget *type_artists_ = nullptr;
  GtkWidget *type_albums_ = nullptr;
  GtkWidget *type_songs_ = nullptr;
  GtkWidget *pretty_covers_btn_ = nullptr;
  GtkWidget *group_button_ = nullptr;
  GtkWidget *configure_button_ = nullptr;
  GtkWidget *list_ = nullptr;
  GtkWidget *progress_ = nullptr;
  GtkWidget *status_ = nullptr;
  GtkWidget *close_ = nullptr;
  int last_search_id_ = -1;
  int progress_max_ = StreamingProgress::kDefaultMaximum;
  CollectionGrouping::Grouping grouping_ = StreamingSearchGroup::DefaultGrouping();
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
  bool pretty_covers_ = true;
  int cover_gen_ = 0;
  std::string pending_query_;
  guint search_timer_ = 0;
  int search_timer_gen_ = 0;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  std::map<std::string, std::string> cover_cache_;
  bool has_error_ = false;
};

#endif
