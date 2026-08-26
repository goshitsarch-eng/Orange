#ifndef STRAWBERRY_QUEUEKEYBOARD_H
#define STRAWBERRY_QUEUEKEYBOARD_H

namespace QueueKeyboard {

inline constexpr unsigned kControlMask = 1u << 2;
inline constexpr unsigned kUp = 0xff52;
inline constexpr unsigned kDown = 0xff54;
inline constexpr unsigned kDelete = 0xffff;

enum class Action { None, Remove, Clear, MoveUp, MoveDown };

// Qt queueview.ui: Delete=Remove, Ctrl+K=Clear, Ctrl+Up=MoveDown, Ctrl+Down=MoveUp.
inline Action FromKey(const unsigned keyval, const unsigned modifiers, const unsigned control_mask = kControlMask) {
  const bool control = (modifiers & control_mask) != 0;
  if (keyval == kDelete) {
    return Action::Remove;
  }
  if (control && (keyval == 'k' || keyval == 'K')) {
    return Action::Clear;
  }
  if (control && keyval == kUp) {
    return Action::MoveDown;
  }
  if (control && keyval == kDown) {
    return Action::MoveUp;
  }
  return Action::None;
}

}  // namespace QueueKeyboard

#endif
