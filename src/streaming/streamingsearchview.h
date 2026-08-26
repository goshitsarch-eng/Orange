#ifndef STRAWBERRY_STREAMINGSEARCHVIEW_H
#define STRAWBERRY_STREAMINGSEARCHVIEW_H

#include "streaming/streamingsearchmodel.h"
#include "streaming/streamingsearchsortmodel.h"
#include "streaming/streamingservices.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>

class StreamingSearchView {
 public:
  using ActivateCallback = std::function<void(const Song &)>;

  explicit StreamingSearchView(StreamingService *service);
  ~StreamingSearchView();

  GtkWidget *widget() const { return widget_; }
  void Search(const std::string &query);
  void SetActivateCallback(ActivateCallback callback);
  StreamingSearchModel *model() { return &model_; }
  const SongList &results() const { return model_.songs(); }

 private:
  void Rebuild();

  StreamingService *service_ = nullptr;
  StreamingSearchModel model_;
  StreamingSearchSortModel sort_model_{&model_};
  ActivateCallback activate_;
  GtkWidget *widget_ = nullptr;
  GtkWidget *search_entry_ = nullptr;
  GtkWidget *type_artists_ = nullptr;
  GtkWidget *type_albums_ = nullptr;
  GtkWidget *type_songs_ = nullptr;
  GtkWidget *list_ = nullptr;
};

#endif
