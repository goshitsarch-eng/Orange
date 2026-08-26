#ifndef STRAWBERRY_COLLECTIONFILTER_H
#define STRAWBERRY_COLLECTIONFILTER_H

#include "collection/collectionitem.h"
#include "core/song.h"

#include <string>

class CollectionFilter {
 public:
  void SetFilterString(const std::string &filter_string);
  const std::string &filter_string() const { return filter_string_; }

  bool Accepts(const Song &song) const;
  bool AcceptsItem(const CollectionItem *item) const;
  SongList FilterSongs(const SongList &songs) const;

 private:
  std::string filter_string_;
};

#endif
