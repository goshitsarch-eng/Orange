#ifndef STRAWBERRY_PLAYLISTLISTSCROLL_H
#define STRAWBERRY_PLAYLISTLISTSCROLL_H

#include "collection/collectiontypeaheadscroll.h"

namespace PlaylistListScroll {

enum class Hint { None, Top, Bottom };

// Qt PlaylistListContainer::CurrentChanged: selectionModel setCurrentIndex + scrollTo (EnsureVisible).
inline Hint FromBounds(double row_y, double row_height, double value, double page) {
  if (row_y < value) {
    return Hint::Top;
  }
  if (row_y + row_height > value + page) {
    return Hint::Bottom;
  }
  return Hint::None;
}

inline double Value(Hint hint, double row_y, double row_height, double value, double page, double lower, double upper) {
  if (hint == Hint::Top) {
    return CollectionTypeAheadScroll::ClampValue(row_y, lower, upper, page);
  }
  if (hint == Hint::Bottom) {
    return CollectionTypeAheadScroll::ClampValue(row_y + row_height - page, lower, upper, page);
  }
  return value;
}

}  // namespace PlaylistListScroll

#endif
