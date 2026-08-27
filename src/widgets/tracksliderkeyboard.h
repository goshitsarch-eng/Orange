#ifndef STRAWBERRY_TRACKSLIDERKEYBOARD_H
#define STRAWBERRY_TRACKSLIDERKEYBOARD_H

namespace TrackSliderKeyboard {

inline constexpr unsigned kLeft = 0xff51;
inline constexpr unsigned kUp = 0xff52;
inline constexpr unsigned kRight = 0xff53;
inline constexpr unsigned kDown = 0xff54;

enum class Action { None, SeekBack, SeekForward };

// Qt TrackSliderSlider::keyPressEvent: Left/Down seek back, Right/Up seek forward.
inline Action FromKey(unsigned keyval) {
  if (keyval == kLeft || keyval == kDown) {
    return Action::SeekBack;
  }
  if (keyval == kRight || keyval == kUp) {
    return Action::SeekForward;
  }
  return Action::None;
}

inline bool ShouldHandle(bool can_seek, Action action) { return can_seek && action != Action::None; }

}  // namespace TrackSliderKeyboard

#endif
