#ifndef STRAWBERRY_DIALOGLISTKEYBOARD_H
#define STRAWBERRY_DIALOGLISTKEYBOARD_H

#include "widgets/listboxkeyboard.h"

namespace DialogListKeyboard {

inline bool IsActivate(unsigned keyval) { return ListBoxKeyboard::FromKey(keyval) == ListBoxKeyboard::Action::Activate; }

inline bool IsMove(unsigned keyval) {
  const ListBoxKeyboard::Action action = ListBoxKeyboard::FromKey(keyval);
  return action == ListBoxKeyboard::Action::MoveUp || action == ListBoxKeyboard::Action::MoveDown || action == ListBoxKeyboard::Action::Home ||
         action == ListBoxKeyboard::Action::End;
}

inline int NextIndex(int current, int count, unsigned keyval) {
  return ListBoxKeyboard::NextIndex(current, count, ListBoxKeyboard::FromKey(keyval));
}

}  // namespace DialogListKeyboard

#endif
