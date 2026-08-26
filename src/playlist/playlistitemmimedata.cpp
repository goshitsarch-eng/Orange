#include "playlist/playlistitemmimedata.h"

PlaylistItemMimeData::PlaylistItemMimeData(const PlaylistItemPtr &item) {
  if (item) {
    items.push_back(item);
  }
}

PlaylistItemMimeData::PlaylistItemMimeData(const PlaylistItemPtrList &items) : items(items) {}

SongList PlaylistItemMimeData::Songs() const {
  SongList songs;
  for (const PlaylistItemPtr &item : items) {
    if (item) {
      songs.push_back(item->EffectiveMetadata());
    }
  }
  return songs;
}
