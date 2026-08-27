#ifndef STRAWBERRY_PLAYLISTAUTOSCROLL_H
#define STRAWBERRY_PLAYLISTAUTOSCROLL_H

#include "playlist/playlist.h"

namespace PlaylistAutoscroll {

inline constexpr int kGraceMs = 30000;

inline bool ShouldScroll(const Playlist::AutoScroll mode, const bool inhibited) {
  if (mode == Playlist::AutoScroll::Always) {
    return true;
  }
  if (mode == Playlist::AutoScroll::Never) {
    return false;
  }
  return !inhibited;
}

inline bool ShouldSkipIfVisible(const bool row_already_visible) { return row_already_visible; }

inline int CenteredOffset(const int row_y, const int row_height, const int viewport_height) {
  return row_y - (viewport_height - row_height) / 2;
}

}  // namespace PlaylistAutoscroll

#endif
