#ifndef STRAWBERRY_PLAYLISTREMOVEITEMSNOTINQUEUE_H
#define STRAWBERRY_PLAYLISTREMOVEITEMSNOTINQUEUE_H

#include <functional>
#include <vector>

namespace PlaylistRemoveItemsNotInQueue {

inline bool KeepRow(int row, int current_row, bool queued) { return row == current_row || queued; }

inline std::vector<int> RowsToRemove(int row_count, int current_row, const std::function<bool(int)> &queued) {
  std::vector<int> rows;
  if (row_count <= 0) {
    return rows;
  }
  bool any_kept = current_row >= 0 && current_row < row_count;
  if (!any_kept && queued) {
    for (int i = 0; i < row_count; ++i) {
      if (queued(i)) {
        any_kept = true;
        break;
      }
    }
  }
  if (!any_kept) {
    rows.reserve(static_cast<size_t>(row_count));
    for (int i = 0; i < row_count; ++i) {
      rows.push_back(i);
    }
    return rows;
  }
  for (int i = 0; i < row_count; ++i) {
    if (!KeepRow(i, current_row, queued && queued(i))) {
      rows.push_back(i);
    }
  }
  return rows;
}

}  // namespace PlaylistRemoveItemsNotInQueue

#endif  // STRAWBERRY_PLAYLISTREMOVEITEMSNOTINQUEUE_H
