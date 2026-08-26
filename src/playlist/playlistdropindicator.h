#ifndef STRAWBERRY_PLAYLISTDROPINDICATOR_H
#define STRAWBERRY_PLAYLISTDROPINDICATOR_H

namespace PlaylistDropIndicator {

inline constexpr int kLineWidth = 2;
inline constexpr int kGradientWidth = 5;

enum class Position { Above, Below, Empty };

struct State {
  int insert_row = -1;
  Position pos = Position::Empty;
  int line_y = -1;
};

inline State FromPointer(const double y, const int row_index, const double row_y, const double row_height, const bool has_rows,
                         const double last_bottom) {
  if (!has_rows) {
    return {0, Position::Empty, 1};
  }
  if (row_index < 0) {
    return {0, Position::Empty, static_cast<int>(last_bottom)};
  }
  if (y < row_y + row_height / 2.0) {
    return {row_index, Position::Above, static_cast<int>(row_y)};
  }
  return {row_index + 1, Position::Below, static_cast<int>(row_y + row_height)};
}

inline bool Active(const State &state) { return state.line_y >= 0 && state.insert_row >= 0; }

inline int InsertRow(const State &state, const int fallback) { return Active(state) ? state.insert_row : fallback; }

}  // namespace PlaylistDropIndicator

#endif
