#ifndef STRAWBERRY_PLAYLISTHEADERSORT_H
#define STRAWBERRY_PLAYLISTHEADERSORT_H

#include "playlist/playlistdelegates.h"

#include <string>

namespace PlaylistHeaderSort {

inline constexpr const char *kAscendingMark = " ▲";
inline constexpr const char *kDescendingMark = " ▼";

struct State {
  PlaylistColumn column = PlaylistColumn::Count;
  bool descending = false;
};

inline bool IsSorted(const PlaylistColumn column, const PlaylistColumn sort_column) {
  return sort_column != PlaylistColumn::Count && column == sort_column;
}

inline bool HasSort(const PlaylistColumn sort_column) { return sort_column != PlaylistColumn::Count; }

inline bool AscendingChecked(const PlaylistColumn column, const PlaylistColumn sort_column, const bool descending) {
  return IsSorted(column, sort_column) && !descending;
}

inline bool DescendingChecked(const PlaylistColumn column, const PlaylistColumn sort_column, const bool descending) {
  return IsSorted(column, sort_column) && descending;
}

inline bool ClearEnabled(const PlaylistColumn sort_column) { return HasSort(sort_column); }

inline std::string Label(const std::string &title, const bool sorted, const bool descending) {
  if (!sorted) {
    return title;
  }
  return title + (descending ? kDescendingMark : kAscendingMark);
}

inline std::string LabelForColumn(const std::string &title, const PlaylistColumn column, const PlaylistColumn sort_column,
                                  const bool descending) {
  return Label(title, IsSorted(column, sort_column), descending);
}

inline bool ShouldApplyExplicit(const PlaylistColumn column, const PlaylistColumn sort_column, const bool descending,
                                const PlaylistSortOrder order) {
  if (order == PlaylistSortOrder::Ascending) {
    return !AscendingChecked(column, sort_column, descending);
  }
  if (order == PlaylistSortOrder::Descending) {
    return !DescendingChecked(column, sort_column, descending);
  }
  if (order == PlaylistSortOrder::Clear) {
    return ClearEnabled(sort_column);
  }
  return true;
}

inline State ApplyOrder(const State current, const PlaylistColumn column, const PlaylistSortOrder order) {
  if (order == PlaylistSortOrder::Clear) {
    return {PlaylistColumn::Count, false};
  }
  if (order == PlaylistSortOrder::Ascending) {
    return {column, false};
  }
  if (order == PlaylistSortOrder::Descending) {
    return {column, true};
  }
  if (current.column == column) {
    return {column, !current.descending};
  }
  return {column, false};
}

inline bool ShouldSortNow(const PlaylistColumn sort_column) { return HasSort(sort_column); }

}  // namespace PlaylistHeaderSort

#endif
