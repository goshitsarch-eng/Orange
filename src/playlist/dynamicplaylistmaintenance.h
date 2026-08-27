#ifndef STRAWBERRY_DYNAMICPLAYLISTMAINTENANCE_H
#define STRAWBERRY_DYNAMICPLAYLISTMAINTENANCE_H

#include "smartplaylists/playlistgenerator.h"

#include <algorithm>

namespace DynamicPlaylistMaintenance {

inline int HistoryLength(int current_row) { return current_row < 0 ? 0 : current_row; }

inline int FutureCount(int row_count, int current_row) {
  if (current_row < 0) {
    return std::max(0, row_count);
  }
  return std::max(0, row_count - current_row - 1);
}

inline int HistoryTrimCount(int history_length, int max_history = PlaylistGenerator::kDefaultDynamicHistory) {
  return std::max(0, history_length - max_history);
}

inline int FutureInsertCount(int history_length, int max_future, int row_count) {
  return std::max(0, history_length + 1 + max_future - row_count);
}

inline bool ShouldClearUndo(bool dynamic, bool advanced_forward) { return dynamic && advanced_forward; }

}  // namespace DynamicPlaylistMaintenance

#endif  // STRAWBERRY_DYNAMICPLAYLISTMAINTENANCE_H
