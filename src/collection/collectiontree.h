#ifndef STRAWBERRY_COLLECTIONTREE_H
#define STRAWBERRY_COLLECTIONTREE_H

#include "collection/collectionitem.h"

#include <set>
#include <string>

namespace CollectionTree {

inline bool IsExpandable(const CollectionItem *item) {
  return item && item->type == CollectionItem::Type::Container && !item->children.empty();
}

inline std::string Key(const CollectionItem *item) {
  if (!item) {
    return {};
  }
  std::string key = item->key.empty() ? item->display_text : item->key;
  key += ":";
  key += std::to_string(item->container_level);
  if (item->type == CollectionItem::Type::Song) {
    key += ":";
    key += item->metadata.url();
  }
  return key;
}

inline bool ShowChildren(const CollectionItem *item, bool filter_active, const std::set<std::string> &expanded) {
  if (!IsExpandable(item)) {
    return false;
  }
  if (filter_active) {
    return true;
  }
  return expanded.find(Key(item)) != expanded.end();
}

inline bool Toggle(std::set<std::string> *expanded, const CollectionItem *item) {
  if (!expanded || !IsExpandable(item)) {
    return false;
  }
  const std::string key = Key(item);
  if (expanded->erase(key) == 0) {
    expanded->insert(key);
    return true;
  }
  return false;
}

inline void CollectExpandableKeys(const CollectionItem *item, std::set<std::string> *keys) {
  if (!item || !keys) {
    return;
  }
  if (IsExpandable(item)) {
    keys->insert(Key(item));
  }
  for (const auto &child : item->children) {
    CollectExpandableKeys(child.get(), keys);
  }
}

inline std::string DragPayload(const SongList &songs) {
  std::string text;
  for (const Song &song : songs) {
    if (song.url().empty()) {
      continue;
    }
    if (!text.empty()) {
      text += "\n";
    }
    text += song.url();
  }
  return text;
}

}  // namespace CollectionTree

#endif
