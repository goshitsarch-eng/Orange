#ifndef STRAWBERRY_PLAYLISTINSERTSCROLL_H
#define STRAWBERRY_PLAYLISTINSERTSCROLL_H

namespace PlaylistInsertScroll {

// Qt PlaylistView::rowsInserted: last inserted row is the last model row.
inline bool AtEnd(int start, int inserted, int row_count_after) {
  return inserted > 0 && start >= 0 && start + inserted == row_count_after;
}

// Qt skips dynamic playlists so the playing track stays visible. Skip load/sort so the start index still matches the view.
inline bool ShouldScroll(bool at_end, bool is_dynamic, bool loading, bool will_sort) {
  return at_end && !is_dynamic && !loading && !will_sort;
}

inline int ScrollRow(bool should_scroll, int start) { return should_scroll ? start : -1; }

}  // namespace PlaylistInsertScroll

#endif
