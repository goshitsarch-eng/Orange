#ifndef STRAWBERRY_STREAMINGCOLLECTIONTREE_H
#define STRAWBERRY_STREAMINGCOLLECTIONTREE_H

#include "collection/collectionitem.h"
#include "collection/collectiontree.h"

#include <set>
#include <string>

namespace StreamingCollectionTree {

inline bool FilterActive(const std::string &filter) { return filter.find_first_not_of(" \t\n\r") != std::string::npos; }

inline bool ShouldExpandAll(bool filter_active) { return filter_active; }

inline bool WalkChildren(const CollectionItem *item, bool filter_active, const std::set<std::string> &expanded) {
  if (!item) {
    return false;
  }
  if (item->type == CollectionItem::Type::Root) {
    return true;
  }
  return CollectionTree::ShowChildren(item, filter_active, expanded);
}

inline int VisibleRowCount(const CollectionItem *item, bool filter_active, const std::set<std::string> &expanded) {
  if (!item) {
    return 0;
  }
  int count = item->type == CollectionItem::Type::Root ? 0 : 1;
  if (WalkChildren(item, filter_active, expanded)) {
    for (const auto &child : item->children) {
      count += VisibleRowCount(child.get(), filter_active, expanded);
    }
  }
  return count;
}

inline int VisibleSongCount(const CollectionItem *item, bool filter_active, const std::set<std::string> &expanded) {
  if (!item) {
    return 0;
  }
  if (item->type == CollectionItem::Type::Song) {
    return 1;
  }
  if (!WalkChildren(item, filter_active, expanded)) {
    return 0;
  }
  int count = 0;
  for (const auto &child : item->children) {
    count += VisibleSongCount(child.get(), filter_active, expanded);
  }
  return count;
}

inline SongList SongsFromItem(const CollectionItem *item) {
  if (!item) {
    return {};
  }
  if (item->type == CollectionItem::Type::Song) {
    return {item->metadata};
  }
  SongList songs;
  for (const auto &child : item->children) {
    const SongList more = SongsFromItem(child.get());
    songs.insert(songs.end(), more.begin(), more.end());
  }
  return songs;
}

inline Song RepresentativeSong(const CollectionItem *item) {
  const SongList songs = SongsFromItem(item);
  return songs.empty() ? Song() : songs.front();
}

inline std::string StatusText(int total_songs) {
  if (total_songs == 1) {
    return "1 item";
  }
  return std::to_string(total_songs) + " items";
}

}  // namespace StreamingCollectionTree

#endif
