#ifndef STRAWBERRY_PLAYLISTEDITORDER_H
#define STRAWBERRY_PLAYLISTEDITORDER_H

#include "playlist/playlistdelegates.h"

#include <vector>

namespace PlaylistEditOrder {

inline constexpr unsigned kTab = 0xff09;
inline constexpr unsigned kISOLeftTab = 0xfe20;

enum class TabAction { None, Next, Previous };

inline TabAction FromKey(unsigned keyval, bool shift) {
  if (keyval == kISOLeftTab) {
    return TabAction::Previous;
  }
  if (keyval == kTab) {
    return shift ? TabAction::Previous : TabAction::Next;
  }
  return TabAction::None;
}

inline std::vector<PlaylistColumn> EditableVisible(const std::vector<PlaylistColumn> &visible) {
  std::vector<PlaylistColumn> out;
  for (PlaylistColumn column : visible) {
    if (PlaylistDelegates::ColumnIsEditable(column)) {
      out.push_back(column);
    }
  }
  return out;
}

struct Cell {
  int row = -1;
  PlaylistColumn column = PlaylistColumn::Title;
  bool valid = false;
};

inline int IndexOf(const std::vector<int> &rows, int row) {
  for (size_t i = 0; i < rows.size(); ++i) {
    if (rows[i] == row) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

inline int IndexOfColumn(const std::vector<PlaylistColumn> &columns, PlaylistColumn column) {
  for (size_t i = 0; i < columns.size(); ++i) {
    if (columns[i] == column) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

inline Cell Next(int row, PlaylistColumn column, const std::vector<int> &rows, const std::vector<PlaylistColumn> &editable) {
  if (rows.empty() || editable.empty()) {
    return {};
  }
  int row_i = IndexOf(rows, row);
  if (row_i < 0) {
    row_i = 0;
  }
  int col_i = IndexOfColumn(editable, column);
  if (col_i < 0) {
    col_i = -1;
  }
  ++col_i;
  if (col_i >= static_cast<int>(editable.size())) {
    col_i = 0;
    row_i = row_i + 1 >= static_cast<int>(rows.size()) ? 0 : row_i + 1;
  }
  return {rows[static_cast<size_t>(row_i)], editable[static_cast<size_t>(col_i)], true};
}

inline Cell Previous(int row, PlaylistColumn column, const std::vector<int> &rows, const std::vector<PlaylistColumn> &editable) {
  if (rows.empty() || editable.empty()) {
    return {};
  }
  int row_i = IndexOf(rows, row);
  if (row_i < 0) {
    row_i = 0;
  }
  int col_i = IndexOfColumn(editable, column);
  if (col_i < 0) {
    col_i = 0;
  }
  --col_i;
  if (col_i < 0) {
    col_i = static_cast<int>(editable.size()) - 1;
    row_i = row_i <= 0 ? static_cast<int>(rows.size()) - 1 : row_i - 1;
  }
  return {rows[static_cast<size_t>(row_i)], editable[static_cast<size_t>(col_i)], true};
}

}  // namespace PlaylistEditOrder

#endif
