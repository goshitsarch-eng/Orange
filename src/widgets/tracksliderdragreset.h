#ifndef STRAWBERRY_TRACKSLIDERDRAGRESET_H
#define STRAWBERRY_TRACKSLIDERDRAGRESET_H

namespace TrackSliderDragReset {

// Qt TrackSliderSlider::changeEvent synthesizes a mouse release when the slider
// is disabled mid-drag so hover after Stop does not resume seeking.
inline bool ShouldResetOnDisable(bool was_sensitive, bool now_sensitive, bool dragging) {
  return was_sensitive && !now_sensitive && dragging;
}

inline bool DraggingAfterPress(bool primary_button, int n_press) { return primary_button && n_press == 1; }

inline bool DraggingAfterRelease() { return false; }

}  // namespace TrackSliderDragReset

#endif
