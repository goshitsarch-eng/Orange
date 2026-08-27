#ifndef STRAWBERRY_PLAYLISTRESTORESCROLL_H
#define STRAWBERRY_PLAYLISTRESTORESCROLL_H

#include <vector>

namespace PlaylistRestoreScroll {

inline int TargetRow(int last_played, int current_row) { return last_played >= 0 ? last_played : current_row; }

inline bool ShouldJump(int row) { return row >= 0; }

// Qt JumpToLastPlayedTrack selects last played without making it playlist current.
inline std::vector<int> SelectionForRestore(int last_played, const std::vector<int> &existing) {
  if (existing.empty() && last_played >= 0) {
    return {last_played};
  }
  return existing;
}

}  // namespace PlaylistRestoreScroll

#endif
