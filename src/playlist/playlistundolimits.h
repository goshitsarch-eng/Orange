#ifndef STRAWBERRY_PLAYLISTUNDOLIMITS_H
#define STRAWBERRY_PLAYLISTUNDOLIMITS_H

#include <string>

namespace PlaylistUndoLimits {

inline constexpr int kUndoItemLimit = 500;

inline bool NeedsClearConfirmation(int row_count) { return row_count > kUndoItemLimit; }

inline const char *ClearConfirmTitle() { return "Clear playlist"; }

inline std::string ClearConfirmBody(int row_count) {
  return "Playlist has " + std::to_string(row_count) +
         " songs, too large to undo, are you sure you want to clear the playlist?";
}

}  // namespace PlaylistUndoLimits

#endif  // STRAWBERRY_PLAYLISTUNDOLIMITS_H
