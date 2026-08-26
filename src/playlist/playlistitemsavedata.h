#ifndef STRAWBERRY_PLAYLISTITEMSAVEDATA_H
#define STRAWBERRY_PLAYLISTITEMSAVEDATA_H

#include "core/song.h"

#include <string>
#include <vector>

class PlaylistItemSaveData {
 public:
  PlaylistItemSaveData() = default;
  explicit PlaylistItemSaveData(const Song &song, const std::string &uuid = {});

  Song::Source source = Song::Source::Unknown;
  std::string uuid;
  int collection_id = -1;
  Song song;
};

using PlaylistItemSaveDataList = std::vector<PlaylistItemSaveData>;

#endif
