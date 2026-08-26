#ifndef STRAWBERRY_PLAYLISTITEMMIMEDATA_H
#define STRAWBERRY_PLAYLISTITEMMIMEDATA_H

#include "playlist/playlistitem.h"

class PlaylistItemMimeData {
 public:
  PlaylistItemMimeData() = default;
  explicit PlaylistItemMimeData(const PlaylistItemPtr &item);
  explicit PlaylistItemMimeData(const PlaylistItemPtrList &items);

  PlaylistItemPtrList items;
  SongList Songs() const;
};

#endif
