#ifndef STRAWBERRY_COLLECTIONMODELUPDATE_H
#define STRAWBERRY_COLLECTIONMODELUPDATE_H

#include "core/song.h"

enum class CollectionModelUpdateType {
  Reset,
  AddSongs,
  RemoveSongs,
  UpdateSongs
};

struct CollectionModelUpdate {
  CollectionModelUpdateType type = CollectionModelUpdateType::Reset;
  SongList songs;
};

#endif
