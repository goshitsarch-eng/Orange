#ifndef STRAWBERRY_MAINWINDOWKEYBOARD_H
#define STRAWBERRY_MAINWINDOWKEYBOARD_H

namespace MainWindowKeyboard {

inline constexpr unsigned kSpace = 0x020;
inline constexpr unsigned kKPSpace = 0xff80;
inline constexpr unsigned kLeft = 0xff51;
inline constexpr unsigned kRight = 0xff53;
inline constexpr unsigned kF5 = 0xffc2;
inline constexpr unsigned kF6 = 0xffc3;
inline constexpr unsigned kF7 = 0xffc4;
inline constexpr unsigned kF8 = 0xffc5;

enum class Action { None, PlayPause, Stop, Previous, Next, SeekBack, SeekForward };

// Qt MainWindow::keyPressEvent: Space plays/pauses, Left/Right seek.
inline Action FromWindowKey(const unsigned keyval) {
  if (keyval == kSpace || keyval == kKPSpace) {
    return Action::PlayPause;
  }
  if (keyval == kLeft) {
    return Action::SeekBack;
  }
  if (keyval == kRight) {
    return Action::SeekForward;
  }
  return Action::None;
}

// Qt mainwindow.ui QAction shortcuts.
inline Action FromAccelKey(const unsigned keyval) {
  if (keyval == kF5) {
    return Action::Previous;
  }
  if (keyval == kF6) {
    return Action::PlayPause;
  }
  if (keyval == kF7) {
    return Action::Stop;
  }
  if (keyval == kF8) {
    return Action::Next;
  }
  return Action::None;
}

inline bool ShouldHandleWindowKey(bool editable_focused) { return !editable_focused; }

inline const char *PlayPauseAccel() { return "F6"; }
inline const char *StopAccel() { return "F7"; }
inline const char *PreviousAccel() { return "F5"; }
inline const char *NextAccel() { return "F8"; }

}  // namespace MainWindowKeyboard

#endif
