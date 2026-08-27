#ifndef STRAWBERRY_PLAYLISTPLAYROW_H
#define STRAWBERRY_PLAYLISTPLAYROW_H

namespace PlaylistPlayRow {

inline int Resolve(int current_row, int last_played_row, int row_count) {
  if (current_row >= 0 && current_row < row_count) {
    return current_row;
  }
  if (last_played_row >= 0 && last_played_row < row_count) {
    return last_played_row;
  }
  return row_count > 0 ? 0 : -1;
}

inline int Remember(int row, int previous_last_played) { return row >= 0 ? row : previous_last_played; }

// Qt Restore assigns last_played_item_index_ only when the stored row is still in range.
inline int RestoreIndex(int last_played, int row_count) { return last_played >= 0 && last_played < row_count ? last_played : -1; }

// Qt InsertItems during Restore does not make the first row current.
inline bool ShouldAssignFirstRowOnInsert(bool loading, int current_row) { return !loading && current_row < 0; }

// Qt album shuffle uses last_played when current_row is still unset after startup restore.
inline int ShuffleReference(int current_row, int last_played_row) { return current_row >= 0 ? current_row : last_played_row; }

}  // namespace PlaylistPlayRow

#endif  // STRAWBERRY_PLAYLISTPLAYROW_H
