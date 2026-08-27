#ifndef STRAWBERRY_DEVICEKEYBOARD_H
#define STRAWBERRY_DEVICEKEYBOARD_H

#include "widgets/listboxkeyboard.h"

#include <cstring>

namespace DeviceKeyboard {

enum class Action { None, Activate, MoveUp, MoveDown, Home, End, Escape, Back, Expand, Collapse, TypeAhead };

inline Action FromKey(unsigned keyval) {
  if (keyval == ListBoxKeyboard::kReturn || keyval == ListBoxKeyboard::kKPEnter) {
    return Action::Activate;
  }
  if (keyval == ListBoxKeyboard::kUp) {
    return Action::MoveUp;
  }
  if (keyval == ListBoxKeyboard::kDown) {
    return Action::MoveDown;
  }
  if (keyval == ListBoxKeyboard::kHome) {
    return Action::Home;
  }
  if (keyval == ListBoxKeyboard::kEnd) {
    return Action::End;
  }
  if (keyval == ListBoxKeyboard::kRight) {
    return Action::Expand;
  }
  if (keyval == ListBoxKeyboard::kLeft) {
    return Action::Collapse;
  }
  if (keyval == ListBoxKeyboard::kEscape || keyval == ListBoxKeyboard::kBackSpace) {
    return keyval == ListBoxKeyboard::kBackSpace ? Action::Back : Action::Escape;
  }
  return Action::None;
}

inline ListBoxKeyboard::Action MoveAction(Action action) {
  switch (action) {
    case Action::MoveUp:
      return ListBoxKeyboard::Action::MoveUp;
    case Action::MoveDown:
      return ListBoxKeyboard::Action::MoveDown;
    case Action::Home:
      return ListBoxKeyboard::Action::Home;
    case Action::End:
      return ListBoxKeyboard::Action::End;
    default:
      return ListBoxKeyboard::Action::None;
  }
}

inline bool IsSpecialRowKind(const char *kind) {
  return kind && (std::strcmp(kind, "back") == 0 || std::strcmp(kind, "add-all") == 0);
}

// Qt DeviceView::contextMenuEvent uses currentIndex(): device menu vs collection/song menu.
constexpr unsigned kMenuKey = 0xff67;
constexpr unsigned kF10Key = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;

inline bool IsMenuTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenuKey || (keyval == kF10Key && (state & kShiftMask) != 0);
}

enum class MenuTarget { None, Device, Song };

inline MenuTarget MenuForSelection(bool device_row, bool song_row) {
  if (device_row) {
    return MenuTarget::Device;
  }
  if (song_row) {
    return MenuTarget::Song;
  }
  return MenuTarget::None;
}

}  // namespace DeviceKeyboard

#endif
