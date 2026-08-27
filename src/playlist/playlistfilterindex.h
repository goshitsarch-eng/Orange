#ifndef STRAWBERRY_PLAYLISTFILTERINDEX_H
#define STRAWBERRY_PLAYLISTFILTERINDEX_H

#include "core/song.h"
#include "playlist/playlistfilter.h"

namespace PlaylistFilterIndex {

inline bool RowVisible(const PlaylistFilter &filter, const Song &song) { return filter.Accepts(song); }

inline int RepeatTrackRow(int current_row, const PlaylistFilter &filter, const Song &current) {
  if (current_row < 0) {
    return -1;
  }
  return RowVisible(filter, current) ? current_row : -1;
}

}  // namespace PlaylistFilterIndex

#endif  // STRAWBERRY_PLAYLISTFILTERINDEX_H
