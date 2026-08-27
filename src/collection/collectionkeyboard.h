#ifndef STRAWBERRY_COLLECTIONKEYBOARD_H
#define STRAWBERRY_COLLECTIONKEYBOARD_H

#include "widgets/listboxkeyboard.h"

namespace CollectionKeyboard {

enum class Action { None, Activate, MoveUp, MoveDown, Home, End, Expand, Collapse, Escape, TypeAhead };

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
  if (keyval == ListBoxKeyboard::kEscape) {
    return Action::Escape;
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

}  // namespace CollectionKeyboard

#endif
