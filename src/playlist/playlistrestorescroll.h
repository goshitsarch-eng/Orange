#ifndef STRAWBERRY_PLAYLISTRESTORESCROLL_H
#define STRAWBERRY_PLAYLISTRESTORESCROLL_H

namespace PlaylistRestoreScroll {

inline int TargetRow(int last_played, int current_row) { return last_played >= 0 ? last_played : current_row; }

inline bool ShouldJump(int row) { return row >= 0; }

}  // namespace PlaylistRestoreScroll

#endif
