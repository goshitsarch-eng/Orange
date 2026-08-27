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

}  // namespace PlaylistPlayRow

#endif  // STRAWBERRY_PLAYLISTPLAYROW_H
