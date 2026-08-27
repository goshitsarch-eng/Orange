#ifndef STRAWBERRY_PLAYLISTEDITCOLUMN_H
#define STRAWBERRY_PLAYLISTEDITCOLUMN_H

#include "playlist/playlisteditorder.h"

#include <vector>

namespace PlaylistEditColumn {

// Qt PlaylistCurrentChanged: keyboard row changes must not reuse the last right-click column.
inline bool ShouldClearLastClicked(int last_clicked_row, int current_row) {
  return last_clicked_row >= 0 && last_clicked_row != current_row;
}

inline bool HasLastClicked(PlaylistColumn column) { return column != PlaylistColumn::Count; }

inline PlaylistColumn DefaultEditColumn(const std::vector<PlaylistColumn> &visible) {
  const std::vector<PlaylistColumn> editable = PlaylistEditOrder::EditableVisible(visible);
  return editable.empty() ? PlaylistColumn::Count : editable.front();
}

inline PlaylistColumn Resolve(PlaylistColumn last_clicked, const std::vector<PlaylistColumn> &visible) {
  if (HasLastClicked(last_clicked) && PlaylistDelegates::ColumnIsEditable(last_clicked)) {
    return last_clicked;
  }
  return DefaultEditColumn(visible);
}

}  // namespace PlaylistEditColumn

#endif
