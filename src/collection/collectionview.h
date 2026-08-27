#ifndef STRAWBERRY_COLLECTIONVIEW_H
#define STRAWBERRY_COLLECTIONVIEW_H

#include "collection/collectioncover.h"
#include "collection/collectionfilter.h"
#include "collection/collectionfocus.h"
#include "collection/collectionkeyboard.h"
#include "collection/collectionmodel.h"

#include <gtk/gtk.h>

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

class AlbumCoverLoader;

class CollectionView {
 public:
  using ActivateCallback = std::function<void(const SongList &)>;
  using EnqueueCallback = std::function<void(const SongList &)>;
  using EmptyCallback = std::function<void()>;
  using FocusFilterCallback = std::function<void(unsigned keyval)>;
  using MenuCallback = std::function<void(double x, double y)>;

  CollectionView();
  ~CollectionView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  CollectionModel *model() { return &model_; }
  CollectionFilter *filter() { return &filter_; }

  void SetModelSongs(const SongList &songs, const CollectionGrouping::Grouping &grouping, bool separate_albums_by_grouping,
                     bool skip_artist_articles, bool skip_album_articles);
  void ApplyUpdate(const CollectionModelUpdate &update);
  void SetFilterString(const std::string &filter);
  void SetActivateCallback(ActivateCallback callback);
  void SetEnqueueCallback(EnqueueCallback callback);
  void SetEmptyCallback(EmptyCallback callback);
  void SetFocusFilterCallback(FocusFilterCallback callback);
  void SetMenuCallback(MenuCallback callback);
  void FilterReturnPressed();
  void FocusAndMove(CollectionKeyboard::Action action);
  void SetCoverLoader(AlbumCoverLoader *loader) { cover_loader_ = loader; }
  void ApplyLook();
  void Rebuild();
  void SaveFocus();
  void RestoreFocus();
  void ExpandAll();
  void CollapseAll();
  void ToggleExpanded(const CollectionItem *item);
  bool IsExpanded(const CollectionItem *item) const;

  SongList SelectedSongs() const;
  const CollectionItem *SelectedItem() const;
  std::vector<const CollectionItem *> SelectedItems() const;

 private:
  void RebuildRows();
  void SelectFocusItem();
  void SelectItem(const CollectionItem *target);
  void AppendItem(GtkWidget *parent, const CollectionItem *item, int depth);
  void LoadCover(GtkWidget *image, const Song &song);
  void SetupRowDrag(GtkWidget *row, const CollectionItem *item);
  void TypeAhead(gunichar ch);
  void ResetTypeAhead();
  void ScrollRowToTop(GtkWidget *row);
  gboolean OnKeyPressed(guint keyval, GdkModifierType state);
  bool ApplyTreeLeft();
  void ActivateRow(GtkListBoxRow *row);
  void HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state);
  GtkWidget *CreateEmptyPlaceholder(bool collection_empty) const;
  void OpenEmptyIfNeeded(GtkListBoxRow *row, guint button);

  CollectionModel model_;
  CollectionFilter filter_;
  CollectionGrouping::Grouping grouping_;
  ActivateCallback activate_;
  EnqueueCallback enqueue_;
  EmptyCallback empty_;
  FocusFilterCallback focus_filter_;
  MenuCallback menu_;
  AlbumCoverLoader *cover_loader_ = nullptr;
  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  std::set<std::string> expanded_;
  CollectionFocus::State focus_;
  std::map<std::string, std::string> cover_cache_;
  bool pretty_covers_ = CollectionSettings::kDefaultPrettyCovers;
  bool auto_open_ = CollectionSettings::kDefaultAutoOpen;
  std::string typeahead_;
  guint typeahead_timeout_id_ = 0;
};

#endif
