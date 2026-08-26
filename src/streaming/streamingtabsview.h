#ifndef STRAWBERRY_STREAMINGTABSVIEW_H
#define STRAWBERRY_STREAMINGTABSVIEW_H

#include "streaming/streamingcollectionstore.h"
#include "streaming/streamingcollectionviewcontainer.h"
#include "streaming/streamingsearchview.h"
#include "streaming/streamingservices.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>

class Database;

class StreamingTabsView {
 public:
  using ActivateCallback = std::function<void(const Song &)>;
  using EnqueueCallback = std::function<void(const SongList &)>;
  using MenuCallback = std::function<void(const SongList &)>;
  using ConfigureCallback = std::function<void()>;

  explicit StreamingTabsView(StreamingService *service, Database *database = nullptr);
  ~StreamingTabsView();

  GtkWidget *widget() const { return widget_; }
  StreamingService *service() const { return service_; }
  void SetActivateCallback(ActivateCallback callback);
  void SetEnqueueCallback(EnqueueCallback callback);
  void SetMenuCallback(MenuCallback callback);
  void SetConfigureCallback(ConfigureCallback callback);
  void SearchForThis(const std::string &query);
  std::string SelectedSearchQuery() const;
  void ReloadSettings();
  void GetArtists();
  void GetAlbums();
  void GetSongs();
  void GetFavorites();
  void AddToCollection(StreamingCollectionStore::List list, const SongList &songs);
  void AbortGetArtists();
  void AbortGetAlbums();
  void AbortGetSongs();
  void AbortGetFavorites();
  StreamingCollectionView *artists_collection_view() const { return artists_->view(); }
  StreamingCollectionView *albums_collection_view() const { return albums_->view(); }
  StreamingCollectionView *songs_collection_view() const { return songs_->view(); }
  StreamingSearchView *search_view() const { return search_.get(); }

 private:
  void HandleActivate(StreamingCollectionView *view, const Song &song);
  void BrowseArtist(StreamingCollectionView *view, const Song &artist);
  void BrowseAlbum(StreamingCollectionView *view, const Song &album);
  void ShowCached(StreamingCollectionView *view, StreamingCollectionStore::List list);
  void PersistList(StreamingCollectionStore::List list, const SongList &songs);
  void BuildFavoriteTypes();
  void SetFavoriteType(StreamingService::FavoriteType type, bool reload);
  void PersistFavoriteType();
  void LoadFavoriteType();

  void ConnectBrowseProgress();

  StreamingService *service_ = nullptr;
  Database *database_ = nullptr;
  ActivateCallback activate_;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  GtkWidget *widget_ = nullptr;
  GtkWidget *stack_ = nullptr;
  std::unique_ptr<StreamingCollectionViewContainer> artists_;
  std::unique_ptr<StreamingCollectionViewContainer> albums_;
  std::unique_ptr<StreamingCollectionViewContainer> songs_;
  std::unique_ptr<StreamingCollectionViewContainer> favorites_;
  std::unique_ptr<StreamingSearchView> search_;
  StreamingService::FavoriteType favorite_type_ = StreamingService::FavoriteType::Songs;
  GtkWidget *fav_artists_ = nullptr;
  GtkWidget *fav_albums_ = nullptr;
  GtkWidget *fav_songs_ = nullptr;
};

#endif
