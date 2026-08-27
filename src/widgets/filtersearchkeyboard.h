#ifndef FILTERSEARCHKEYBOARD_H
#define FILTERSEARCHKEYBOARD_H

#include "widgets/listboxkeyboard.h"

namespace FilterSearchKeyboard {

enum class Action { None, Activate, MoveUp, MoveDown, PageUp, PageDown, Clear, FocusFilter };

inline Action FromSearchKey(const unsigned keyval) {
  if (keyval == ListBoxKeyboard::kReturn || keyval == ListBoxKeyboard::kKPEnter) {
    return Action::Activate;
  }
  if (keyval == ListBoxKeyboard::kUp) {
    return Action::MoveUp;
  }
  if (keyval == ListBoxKeyboard::kDown) {
    return Action::MoveDown;
  }
  if (keyval == ListBoxKeyboard::kPageUp || keyval == ListBoxKeyboard::kKPPageUp) {
    return Action::PageUp;
  }
  if (keyval == ListBoxKeyboard::kPageDown || keyval == ListBoxKeyboard::kKPPageDown) {
    return Action::PageDown;
  }
  if (keyval == ListBoxKeyboard::kEscape) {
    return Action::Clear;
  }
  return Action::None;
}

inline Action FromTreeKey(const unsigned keyval) {
  if (keyval == ListBoxKeyboard::kBackSpace || keyval == ListBoxKeyboard::kEscape) {
    return Action::FocusFilter;
  }
  return Action::None;
}

inline ListBoxKeyboard::Action MoveAction(const Action action) {
  if (action == Action::MoveUp) {
    return ListBoxKeyboard::Action::MoveUp;
  }
  if (action == Action::MoveDown) {
    return ListBoxKeyboard::Action::MoveDown;
  }
  if (action == Action::PageUp) {
    return ListBoxKeyboard::Action::PageUp;
  }
  if (action == Action::PageDown) {
    return ListBoxKeyboard::Action::PageDown;
  }
  return ListBoxKeyboard::Action::None;
}

// Qt PlaylistContainer::eventFilter forwards Down / PageUp / PageDown from the filter to the playlist.
inline bool ForwardsToView(const Action action) { return MoveAction(action) != ListBoxKeyboard::Action::None; }

}  // namespace FilterSearchKeyboard

#endif  // FILTERSEARCHKEYBOARD_H
