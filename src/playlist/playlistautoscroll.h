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

// Qt PlaylistView::showEvent restarts the glow timer and MaybeAutoscroll(Maybe).
inline bool ShouldRunOnShow() { return true; }

inline Playlist::AutoScroll ShowMode() { return Playlist::AutoScroll::Maybe; }

inline bool ShouldRestartGlowOnShow(const bool currently_glowing) { return currently_glowing; }

// Qt PlaylistView::hideEvent stops the glow timer so it is idle while hidden.
inline bool ShouldStopGlowOnHide() { return true; }

}  // namespace PlaylistAutoscroll

#endif
