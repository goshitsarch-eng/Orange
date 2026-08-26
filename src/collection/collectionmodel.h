#ifndef STRAWBERRY_COLLECTIONMODEL_H
#define STRAWBERRY_COLLECTIONMODEL_H

#include "collection/collectionfilteroptions.h"
#include "collection/collectiongrouping.h"
#include "collection/collectionitem.h"
#include "collection/collectionmodelupdate.h"

#include <memory>
#include <string>

class CollectionBackend;

class CollectionModel {
 public:
  explicit CollectionModel(CollectionBackend *backend = nullptr);

  CollectionBackend *backend() const { return backend_; }
  CollectionItem *root() const { return root_.get(); }

  void Reset(const SongList &songs, const CollectionGrouping::Grouping &grouping, bool separate_albums_by_grouping,
             bool skip_artist_articles, bool skip_album_articles);
  void ApplyUpdate(const CollectionModelUpdate &update);

  int TotalSongs() const { return total_songs_; }
  int TotalArtists() const { return total_artists_; }
  int TotalAlbums() const { return total_albums_; }
  const CollectionGrouping::Grouping &grouping() const { return grouping_; }
  SongList Songs() const;
  SongList SongsFromItem(const CollectionItem *item) const;

 private:
  void AppendNode(CollectionItem *parent, const CollectionGrouping::Node &node, int level);
  static void Count(const CollectionItem *item, int *songs, int *artists, int *albums);

  CollectionBackend *backend_ = nullptr;
  CollectionGrouping::Grouping grouping_;
  std::unique_ptr<CollectionItem> root_;
  int total_songs_ = 0;
  int total_artists_ = 0;
  int total_albums_ = 0;
};

#endif
