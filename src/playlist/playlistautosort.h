#ifndef STRAWBERRY_PLAYLISTAUTOSORT_H
#define STRAWBERRY_PLAYLISTAUTOSORT_H

#include "playlist/playlistdelegates.h"

namespace PlaylistAutoSort {

inline bool ShouldSort(bool auto_sort, bool loading, PlaylistColumn column) {
  return auto_sort && !loading && column != PlaylistColumn::Count;
}

}  // namespace PlaylistAutoSort

#endif  // STRAWBERRY_PLAYLISTAUTOSORT_H
