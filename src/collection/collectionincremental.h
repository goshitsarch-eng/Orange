#ifndef STRAWBERRY_COLLECTIONINCREMENTAL_H
#define STRAWBERRY_COLLECTIONINCREMENTAL_H

#include "collection/collectionmodelupdate.h"

namespace CollectionIncremental {

inline bool ShouldApply(bool scanning) { return !scanning; }

inline CollectionModelUpdate Make(CollectionModelUpdateType type, const SongList &songs) {
  CollectionModelUpdate update;
  update.type = type;
  update.songs = songs;
  return update;
}

}  // namespace CollectionIncremental

#endif  // STRAWBERRY_COLLECTIONINCREMENTAL_H
