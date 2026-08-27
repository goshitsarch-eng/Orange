#ifndef STRAWBERRY_COLLECTIONITEM_H
#define STRAWBERRY_COLLECTIONITEM_H

#include "core/song.h"

#include <memory>
#include <string>
#include <vector>

class CollectionItem {
 public:
  enum class Type {
    Root,
    Divider,
    Container,
    Song,
    LoadingIndicator
  };

  CollectionItem() = default;
  explicit CollectionItem(Type type, CollectionItem *parent = nullptr);

  Type type = Type::Root;
  int container_level = -1;
  Song metadata;
  std::string key;
  std::string display_text;
  std::string sort_text;
  std::string divider_key;
  CollectionItem *parent = nullptr;
  CollectionItem *compilation_artist_node = nullptr;
  std::vector<std::unique_ptr<CollectionItem>> children;

  CollectionItem *AddChild(Type child_type);
  SongList Songs() const;
};

#endif
