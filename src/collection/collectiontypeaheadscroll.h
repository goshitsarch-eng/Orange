#ifndef STRAWBERRY_COLLECTIONTYPEAHEADSCROLL_H
#define STRAWBERRY_COLLECTIONTYPEAHEADSCROLL_H

#include <algorithm>

namespace CollectionTypeAheadScroll {

// Qt CollectionView::scrollTo uses PositionAtTop while is_in_keyboard_search_.
inline bool ForceTop(bool keyboard_search) { return keyboard_search; }

inline double ClampValue(double value, double lower, double upper, double page_size) {
  const double max_value = std::max(lower, upper - page_size);
  return std::min(max_value, std::max(lower, value));
}

inline double PositionAtTop(double row_y, double lower, double upper, double page_size) {
  return ClampValue(row_y, lower, upper, page_size);
}

}  // namespace CollectionTypeAheadScroll

#endif
