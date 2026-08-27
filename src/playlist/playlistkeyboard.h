#ifndef STRAWBERRY_PLAYLISTKEYBOARD_H
#define STRAWBERRY_PLAYLISTKEYBOARD_H

namespace PlaylistKeyboard {

inline constexpr unsigned kSpace = 0x020;
inline constexpr unsigned kKPSpace = 0xff80;
inline constexpr unsigned kLeft = 0xff51;
inline constexpr unsigned kRight = 0xff53;
inline constexpr unsigned kControlMask = 1u << 2;

enum class Action { None, PlayPause, SeekBack, SeekForward };

inline Action FromKey(const unsigned keyval, const unsigned modifiers, const unsigned control_mask = kControlMask) {
  if (keyval == kLeft) {
    return Action::SeekBack;
  }
  if (keyval == kRight) {
    return Action::SeekForward;
  }
  if ((modifiers & control_mask) == 0 && (keyval == kSpace || keyval == kKPSpace)) {
    return Action::PlayPause;
  }
  return Action::None;
}

}  // namespace PlaylistKeyboard

#endif
