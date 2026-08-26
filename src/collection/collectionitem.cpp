#include "collection/collectionitem.h"

CollectionItem::CollectionItem(Type type, CollectionItem *parent) : type(type), parent(parent) {}

CollectionItem *CollectionItem::AddChild(Type child_type) {
  auto child = std::make_unique<CollectionItem>(child_type, this);
  CollectionItem *raw = child.get();
  children.push_back(std::move(child));
  return raw;
}

SongList CollectionItem::Songs() const {
  if (type == Type::Song) {
    return metadata.is_valid() ? SongList{metadata} : SongList{};
  }
  SongList songs;
  for (const auto &child : children) {
    const SongList child_songs = child->Songs();
    songs.insert(songs.end(), child_songs.begin(), child_songs.end());
  }
  return songs;
}
