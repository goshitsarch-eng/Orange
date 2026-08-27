#ifndef STRAWBERRY_PLAYLISTRATINGCLICK_H
#define STRAWBERRY_PLAYLISTRATINGCLICK_H

#include "playlist/playlistdelegates.h"
#include "widgets/ratingpainter.h"

namespace PlaylistRatingClick {

inline bool IsRatingColumn(PlaylistColumn column) { return column == PlaylistColumn::Rating; }

inline bool CanApply(bool locked) { return !locked; }

inline float RatingFromClick(int x, int width) { return RatingPainter::RatingForPos(x, width); }

inline bool ShouldRate(PlaylistColumn column, bool locked, int x, int width, float *rating) {
  if (!IsRatingColumn(column) || !CanApply(locked) || !rating) {
    return false;
  }
  *rating = RatingFromClick(x, width);
  return *rating >= 0.0f;
}

}  // namespace PlaylistRatingClick

#endif
