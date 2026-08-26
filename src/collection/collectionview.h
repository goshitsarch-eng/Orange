#ifndef STRAWBERRY_COLLECTIONVIEW_H
#define STRAWBERRY_COLLECTIONVIEW_H

#include "collection/collectionfilter.h"
#include "collection/collectionmodel.h"

#include <gtk/gtk.h>

#include <functional>
#include <set>
#include <string>
#include <vector>

class CollectionView {
 public:
  using ActivateCallback = std::function<void(const SongList &)>;
  using MenuCallback = std::function<void(double x, double y)>;

  CollectionView();
  ~CollectionView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  CollectionModel *model() { return &model_; }
  CollectionFilter *filter() { return &filter_; }

  void SetModelSongs(const SongList &songs, const CollectionGrouping::Grouping &grouping, bool separate_albums_by_grouping,
                     bool skip_artist_articles, bool skip_album_articles);
  void SetFilterString(const std::string &filter);
  void SetActivateCallback(ActivateCallback callback);
  void SetMenuCallback(MenuCallback callback);
  void Rebuild();
  void ExpandAll();
  void CollapseAll();
  void ToggleExpanded(const CollectionItem *item);
  bool IsExpanded(const CollectionItem *item) const;

  SongList SelectedSongs() const;
  const CollectionItem *SelectedItem() const;
  std::vector<const CollectionItem *> SelectedItems() const;

 private:
  void AppendItem(GtkWidget *parent, const CollectionItem *item, int depth);
  void SetupRowDrag(GtkWidget *row, const CollectionItem *item);
  void TypeAhead(gunichar ch);
  void ResetTypeAhead();
  gboolean OnKeyPressed(guint keyval);
  void ActivateRow(GtkListBoxRow *row);

  CollectionModel model_;
  CollectionFilter filter_;
  ActivateCallback activate_;
  MenuCallback menu_;
  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  std::set<std::string> expanded_;
  std::string typeahead_;
  guint typeahead_timeout_id_ = 0;
};

#endif
