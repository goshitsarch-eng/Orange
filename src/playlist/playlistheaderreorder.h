#ifndef STRAWBERRY_PLAYLISTHEADERREORDER_H
#define STRAWBERRY_PLAYLISTHEADERREORDER_H

#include "playlist/playlistdelegates.h"
#include "playlist/playlistcolumnwidths.h"

#include <vector>

namespace PlaylistHeaderReorder {

enum class DragMode { None, Resize, Reorder };

inline DragMode ModeAt(const bool on_resize_handle, const PlaylistColumn column) {
  if (on_resize_handle) {
    return DragMode::Resize;
  }
  if (column != PlaylistColumn::Count) {
    return DragMode::Reorder;
  }
  return DragMode::None;
}

inline int VisualIndex(const std::vector<PlaylistColumn> &visible, const PlaylistColumn column) {
  for (size_t i = 0; i < visible.size(); ++i) {
    if (visible[i] == column) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

inline std::vector<PlaylistColumn> OrderAfterMove(std::vector<PlaylistColumn> visible, const PlaylistColumn source, int dest_visual) {
  const int from = VisualIndex(visible, source);
  if (from < 0 || dest_visual < 0 || dest_visual >= static_cast<int>(visible.size()) || from == dest_visual) {
    return visible;
  }
  const PlaylistColumn column = visible[static_cast<size_t>(from)];
  visible.erase(visible.begin() + from);
  if (dest_visual > from) {
    --dest_visual;
  }
  visible.insert(visible.begin() + dest_visual, column);
  return visible;
}

inline bool ShouldApplyReorder(const PlaylistColumn source, const PlaylistColumn hover) {
  return source != PlaylistColumn::Count && hover != PlaylistColumn::Count && hover != source;
}

inline bool ShouldStartReorder(const double x, const double column_x, const double column_width) {
  return !PlaylistColumnWidths::OnResizeHandleAbsolute(x, column_x, column_width);
}

}  // namespace PlaylistHeaderReorder

#endif
