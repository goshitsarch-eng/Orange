#ifndef STRAWBERRY_PLAYLISTRATINGHOVER_H
#define STRAWBERRY_PLAYLISTRATINGHOVER_H

#include "playlist/playlistratingclick.h"
#include "widgets/ratingpainter.h"

#include <algorithm>
#include <string>
#include <vector>

namespace PlaylistRatingHover {

// Qt PlaylistView::RatingHoverIn is skipped when the view is read-only or the rating column is locked.
inline bool ShouldHover(PlaylistColumn column, bool locked, bool read_only) {
  return PlaylistRatingClick::IsRatingColumn(column) && PlaylistRatingClick::CanApply(locked) && !read_only;
}

inline float PreviewRating(int x, int width) { return PlaylistRatingClick::RatingFromClick(x, width); }

inline bool IsActive(float preview) { return preview >= 0.0f; }

// Qt RatingItemDelegate::set_mouse_over also previews other selected rating cells.
inline bool ShouldPreviewRow(int row, int hovered_row, const std::vector<int> &selected) {
  if (hovered_row < 0) {
    return false;
  }
  if (row == hovered_row) {
    return true;
  }
  if (std::find(selected.begin(), selected.end(), hovered_row) == selected.end()) {
    return false;
  }
  return std::find(selected.begin(), selected.end(), row) != selected.end();
}

inline std::string DisplayText(float stored_rating, float hover_rating, bool previewing) {
  return RatingPainter::Stars(previewing ? hover_rating : stored_rating);
}

inline bool ShouldClear(bool over_rating) { return !over_rating; }

inline const char *CursorName() { return "pointer"; }

}  // namespace PlaylistRatingHover

#endif
