#ifndef STRAWBERRY_COLLECTIONVIEW_H
#define STRAWBERRY_COLLECTIONVIEW_H

#include "collection/collectionfilter.h"
#include "collection/collectionmodel.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>

class CollectionView {
 public:
  using ActivateCallback = std::function<void(const SongList &)>;

  CollectionView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  CollectionModel *model() { return &model_; }
  CollectionFilter *filter() { return &filter_; }

  void SetModelSongs(const SongList &songs, const CollectionGrouping::Grouping &grouping, bool separate_albums_by_grouping,
                     bool skip_artist_articles, bool skip_album_articles);
  void SetFilterString(const std::string &filter);
  void SetActivateCallback(ActivateCallback callback);
  void Rebuild();

 private:
  void AppendItem(GtkWidget *parent, const CollectionItem *item, int depth);

  CollectionModel model_;
  CollectionFilter filter_;
  ActivateCallback activate_;
  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
};

#endif
