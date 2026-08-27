#ifndef STRAWBERRY_PLAYLISTREMOVESELECT_H
#define STRAWBERRY_PLAYLISTREMOVESELECT_H

#include <set>
#include <vector>

namespace PlaylistRemoveSelect {

// Qt PlaylistView::RemoveSelected: keep the row after the last selected range.
inline int NextRow(const std::vector<int> &selected, int row_count_before) {
  if (selected.empty() || row_count_before <= 0) {
    return -1;
  }
  std::set<int> unique;
  for (int row : selected) {
    if (row >= 0 && row < row_count_before) {
      unique.insert(row);
    }
  }
  if (unique.empty()) {
    return -1;
  }
  const int last_row = (selected.back() >= 0 && selected.back() < row_count_before) ? selected.back() : *unique.rbegin();
  int removed_above = 0;
  for (int row : unique) {
    if (row < last_row) {
      ++removed_above;
    }
  }
  const int remaining = row_count_before - static_cast<int>(unique.size());
  if (remaining <= 0) {
    return -1;
  }
  const int next = last_row - removed_above;
  if (next >= 0 && next < remaining) {
    return next;
  }
  return remaining - 1;
}

}  // namespace PlaylistRemoveSelect

#endif
