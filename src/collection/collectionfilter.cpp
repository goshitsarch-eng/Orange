#include "collection/collectionfilter.h"

#include "filterparser/filterparser.h"

void CollectionFilter::SetFilterString(const std::string &filter_string) { filter_string_ = filter_string; }

bool CollectionFilter::Accepts(const Song &song) const {
  if (filter_string_.empty()) {
    return true;
  }
  return FilterParser(filter_string_).Matches(song);
}

bool CollectionFilter::AcceptsItem(const CollectionItem *item) const {
  if (!item) {
    return false;
  }
  if (filter_string_.empty()) {
    return true;
  }
  if (item->type == CollectionItem::Type::LoadingIndicator || item->type == CollectionItem::Type::Divider) {
    return true;
  }
  if (item->type == CollectionItem::Type::Song) {
    return item->metadata.is_valid() && Accepts(item->metadata);
  }
  for (const auto &child : item->children) {
    if (AcceptsItem(child.get())) {
      return true;
    }
  }
  return false;
}

SongList CollectionFilter::FilterSongs(const SongList &songs) const {
  if (filter_string_.empty()) {
    return songs;
  }
  SongList visible;
  for (const Song &song : songs) {
    if (Accepts(song)) {
      visible.push_back(song);
    }
  }
  return visible;
}
