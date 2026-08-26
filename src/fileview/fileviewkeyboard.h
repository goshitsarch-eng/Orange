#ifndef STRAWBERRY_FILEVIEWKEYBOARD_H
#define STRAWBERRY_FILEVIEWKEYBOARD_H

#include "widgets/listboxkeyboard.h"

namespace FileViewKeyboard {

enum class Action { None, Activate, UpDir, HistoryBack, HistoryForward, Home, First, Last, MoveUp, MoveDown, TypeAhead };

inline Action FromKey(unsigned keyval, bool alt) {
  if (keyval == ListBoxKeyboard::kReturn || keyval == ListBoxKeyboard::kKPEnter) {
    return Action::Activate;
  }
  if (alt && keyval == ListBoxKeyboard::kUp) {
    return Action::UpDir;
  }
  if (alt && keyval == ListBoxKeyboard::kLeft) {
    return Action::HistoryBack;
  }
  if (alt && keyval == ListBoxKeyboard::kRight) {
    return Action::HistoryForward;
  }
  if (alt && keyval == ListBoxKeyboard::kHome) {
    return Action::Home;
  }
  if (keyval == ListBoxKeyboard::kBackSpace) {
    return Action::HistoryBack;
  }
  if (keyval == ListBoxKeyboard::kUp) {
    return Action::MoveUp;
  }
  if (keyval == ListBoxKeyboard::kDown) {
    return Action::MoveDown;
  }
  if (keyval == ListBoxKeyboard::kHome) {
    return Action::First;
  }
  if (keyval == ListBoxKeyboard::kEnd) {
    return Action::Last;
  }
  return Action::None;
}

inline Action ResolveHistoryBack(Action action, bool can_back) {
  if (action == Action::HistoryBack && !can_back) {
    return Action::UpDir;
  }
  return action;
}

}  // namespace FileViewKeyboard

#endif
