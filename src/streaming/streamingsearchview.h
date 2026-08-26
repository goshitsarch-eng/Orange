#ifndef STRAWBERRY_STREAMINGSEARCHVIEW_H
#define STRAWBERRY_STREAMINGSEARCHVIEW_H

#include "streaming/streamingsearchmodel.h"
#include "streaming/streamingsearchsortmodel.h"
#include "streaming/streamingservices.h"

#include <gtk/gtk.h>

#include <functional>
#include <map>
#include <memory>
#include <string>

class StreamingSearchView {
 public:
  using ActivateCallback = std::function<void(const Song &)>;
  using MenuCallback = std::function<void(const SongList &)>;

  explicit StreamingSearchView(StreamingService *service);
  ~StreamingSearchView();

  GtkWidget *widget() const { return widget_; }
  void Search(const std::string &query);
  void SetActivateCallback(ActivateCallback callback);
  void SetMenuCallback(MenuCallback callback);
  StreamingSearchModel *model() { return &model_; }
  const SongList &results() const { return model_.songs(); }
  SongList SelectedSongs() const;

 private:
  void Rebuild();
  void SetupRowDrag(GtkWidget *row, const Song &song);
  void LoadCover(GtkWidget *image, const Song &song);
  void PersistPrettyCovers();
  gboolean OnKeyPressed(guint keyval);
  void ResetTypeAhead();

  StreamingService *service_ = nullptr;
  StreamingSearchModel model_;
  StreamingSearchSortModel sort_model_{&model_};
  ActivateCallback activate_;
  MenuCallback menu_;
  GtkWidget *widget_ = nullptr;
  GtkWidget *search_entry_ = nullptr;
  GtkWidget *type_artists_ = nullptr;
  GtkWidget *type_albums_ = nullptr;
  GtkWidget *type_songs_ = nullptr;
  GtkWidget *pretty_covers_btn_ = nullptr;
  GtkWidget *list_ = nullptr;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
  bool pretty_covers_ = true;
  int cover_gen_ = 0;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  std::map<std::string, std::string> cover_cache_;
};

#endif
