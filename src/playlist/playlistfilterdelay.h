#ifndef STRAWBERRY_PLAYLISTFILTERDELAY_H
#define STRAWBERRY_PLAYLISTFILTERDELAY_H

namespace PlaylistFilterDelay {

inline constexpr int kFilterDelayMs = 100;
inline constexpr int kFilterDelayPlaylistSizeThreshold = 5000;

inline bool ShouldDelay(int row_count, bool filter_empty) { return row_count >= kFilterDelayPlaylistSizeThreshold && !filter_empty; }

inline bool ShouldJumpToPlaying(int current_row) { return current_row >= 0; }

}  // namespace PlaylistFilterDelay

#endif  // STRAWBERRY_PLAYLISTFILTERDELAY_H
