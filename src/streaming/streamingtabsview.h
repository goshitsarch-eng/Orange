#ifndef STRAWBERRY_STREAMINGTABSVIEW_H
#define STRAWBERRY_STREAMINGTABSVIEW_H

#include "streaming/streamingcollectionviewcontainer.h"
#include "streaming/streamingsearchview.h"
#include "streaming/streamingservices.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>

class StreamingTabsView {
 public:
  using ActivateCallback = std::function<void(const Song &)>;

  explicit StreamingTabsView(StreamingService *service);
  ~StreamingTabsView();

  GtkWidget *widget() const { return widget_; }
  void SetActivateCallback(ActivateCallback callback);
  void ReloadSettings();
  void GetArtists();
  void GetAlbums();
  void GetSongs();
  void GetFavorites();
  StreamingCollectionView *artists_collection_view() const { return artists_->view(); }
  StreamingCollectionView *albums_collection_view() const { return albums_->view(); }
  StreamingCollectionView *songs_collection_view() const { return songs_->view(); }
  StreamingSearchView *search_view() const { return search_.get(); }

 private:
  StreamingService *service_ = nullptr;
  ActivateCallback activate_;
  GtkWidget *widget_ = nullptr;
  std::unique_ptr<StreamingCollectionViewContainer> artists_;
  std::unique_ptr<StreamingCollectionViewContainer> albums_;
  std::unique_ptr<StreamingCollectionViewContainer> songs_;
  std::unique_ptr<StreamingCollectionViewContainer> favorites_;
  std::unique_ptr<StreamingSearchView> search_;
};

#endif
