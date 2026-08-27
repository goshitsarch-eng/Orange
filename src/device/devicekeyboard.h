#ifndef STRAWBERRY_DEVICEKEYBOARD_H
#define STRAWBERRY_DEVICEKEYBOARD_H

#include "widgets/listboxkeyboard.h"

#include <cstring>

namespace DeviceKeyboard {

enum class Action { None, Activate, MoveUp, MoveDown, Home, End, Escape, Back, TypeAhead };

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

}  // namespace DeviceKeyboard

#endif
