#ifndef STRAWBERRY_LISTBOXKEYBOARD_H
#define STRAWBERRY_LISTBOXKEYBOARD_H

#include "utilities/strutils.h"

#include <string>
#include <vector>

namespace ListBoxKeyboard {

constexpr unsigned kReturn = 0xff0d;
constexpr unsigned kKPEnter = 0xff8d;
constexpr unsigned kUp = 0xff52;
constexpr unsigned kDown = 0xff54;
constexpr unsigned kLeft = 0xff51;
constexpr unsigned kRight = 0xff53;
constexpr unsigned kHome = 0xff50;
constexpr unsigned kEnd = 0xff57;
constexpr unsigned kEscape = 0xff1b;
constexpr unsigned kBackSpace = 0xff08;
constexpr unsigned kDelete = 0xffff;

enum class Action { None, Activate, MoveUp, MoveDown, Home, End, Escape, TypeAhead, Delete, Backspace };

inline Action FromKey(unsigned keyval) {
  if (keyval == kReturn || keyval == kKPEnter) {
    return Action::Activate;
  }
  if (keyval == kUp) {
    return Action::MoveUp;
  }
  if (keyval == kDown) {
    return Action::MoveDown;
  }
  if (keyval == kHome) {
    return Action::Home;
  }
  if (keyval == kEnd) {
    return Action::End;
  }
  if (keyval == kEscape) {
    return Action::Escape;
  }
  if (keyval == kDelete) {
    return Action::Delete;
  }
  if (keyval == kBackSpace) {
    return Action::Backspace;
  }
  return Action::None;
}

inline int NextIndex(int current, int count, Action action) {
  if (count <= 0) {
    return -1;
  }
  if (action == Action::Home) {
    return 0;
  }
  if (action == Action::End) {
    return count - 1;
  }
  const int start = current < 0 ? 0 : current;
  if (action == Action::MoveUp) {
    return start <= 0 ? count - 1 : start - 1;
  }
  if (action == Action::MoveDown) {
    return start >= count - 1 ? 0 : start + 1;
  }
  return start;
}

inline int FirstPrefixIndex(const std::vector<std::string> &labels, const std::string &needle) {
  if (needle.empty()) {
    return -1;
  }
  const std::string n = StrUtils::ToLower(needle);
  for (size_t i = 0; i < labels.size(); ++i) {
    if (StrUtils::StartsWith(StrUtils::ToLower(labels[i]), n)) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace ListBoxKeyboard

#endif
