#include "collection/collectionmodel.h"

#include "collection/collectionbackend.h"

CollectionModel::CollectionModel(CollectionBackend *backend) : backend_(backend), root_(std::make_unique<CollectionItem>(CollectionItem::Type::Root)) {}

void CollectionModel::AppendNode(CollectionItem *parent, const CollectionGrouping::Node &node, int level) {
  if (!parent) {
    return;
  }
  if (node.songs.empty() && node.children.empty() && node.display.empty()) {
    for (const auto &child : node.children) {
      AppendNode(parent, child, level);
    }
    return;
  }
  if (!node.children.empty() || !node.songs.empty()) {
    CollectionItem *container = parent;
    if (!node.display.empty() || !node.key.empty()) {
      container = parent->AddChild(CollectionItem::Type::Container);
      container->container_level = level;
      container->key = node.key;
      container->display_text = node.display;
      container->sort_text = node.sort;
    }
    for (const auto &child : node.children) {
      AppendNode(container, child, level + 1);
    }
    for (const Song &song : node.songs) {
      CollectionItem *item = container->AddChild(CollectionItem::Type::Song);
      item->metadata = song;
      item->display_text = song.PrettyTitle();
      item->sort_text = song.title();
    }
  }
}

void CollectionModel::Reset(const SongList &songs, const CollectionGrouping::Grouping &grouping, bool separate_albums_by_grouping,
                            bool skip_artist_articles, bool skip_album_articles) {
  grouping_ = grouping;
  root_ = std::make_unique<CollectionItem>(CollectionItem::Type::Root);
  const CollectionGrouping::Node tree =
      CollectionGrouping::BuildTree(songs, grouping, separate_albums_by_grouping, skip_artist_articles, skip_album_articles);
  for (const auto &child : tree.children) {
    AppendNode(root_.get(), child, 0);
  }
  for (const Song &song : tree.songs) {
    CollectionItem *item = root_->AddChild(CollectionItem::Type::Song);
    item->metadata = song;
    item->display_text = song.PrettyTitle();
    item->sort_text = song.title();
  }
  total_songs_ = 0;
  total_artists_ = 0;
  total_albums_ = 0;
  Count(root_.get(), &total_songs_, &total_artists_, &total_albums_);
}

void CollectionModel::ApplyUpdate(const CollectionModelUpdate &update) {
  if (update.type == CollectionModelUpdateType::Reset) {
    Reset(update.songs, CollectionGrouping::LoadCurrent(), CollectionGrouping::SeparateAlbumsByGrouping(), true, false);
  }
}

SongList CollectionModel::Songs() const { return root_ ? root_->Songs() : SongList{}; }

SongList CollectionModel::SongsFromItem(const CollectionItem *item) const { return item ? item->Songs() : SongList{}; }

void CollectionModel::Count(const CollectionItem *item, int *songs, int *artists, int *albums) {
  if (!item) {
    return;
  }
  if (item->type == CollectionItem::Type::Song) {
    ++*songs;
    return;
  }
  if (item->type == CollectionItem::Type::Container && item->container_level == 0) {
    ++*artists;
  }
  if (item->type == CollectionItem::Type::Container && item->container_level == 1) {
    ++*albums;
  }
  for (const auto &child : item->children) {
    Count(child.get(), songs, artists, albums);
  }
}
