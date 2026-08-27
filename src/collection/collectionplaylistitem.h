#ifndef STRAWBERRY_COLLECTIONPLAYLISTITEM_H
#define STRAWBERRY_COLLECTIONPLAYLISTITEM_H

#include "core/song.h"

#include <string>

class CollectionPlaylistItem {
 public:
  CollectionPlaylistItem() = default;
  explicit CollectionPlaylistItem(const Song &song);

  const Song &song() const { return song_; }
  int collection_id() const { return song_.id(); }
  const std::string &url() const { return song_.url(); }
  std::string DisplayText() const;

 private:
  Song song_;
};

#endif
