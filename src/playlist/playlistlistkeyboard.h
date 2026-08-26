#ifndef STRAWBERRY_PLAYLISTLISTKEYBOARD_H
#define STRAWBERRY_PLAYLISTLISTKEYBOARD_H

#include "playlist/playlistlistdrop.h"
#include "widgets/listboxkeyboard.h"

#include <string>
#include <vector>

namespace PlaylistListKeyboard {

enum class Action { None, Activate, MoveUp, MoveDown, Home, End, Expand, Collapse, Escape, Delete };

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
  if (keyval == ListBoxKeyboard::kDelete) {
    return Action::Delete;
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

inline std::vector<std::string> RowLabels(const std::vector<PlaylistListDrop::Row> &rows) {
  std::vector<std::string> labels;
  labels.reserve(rows.size());
  for (const PlaylistListDrop::Row &row : rows) {
    labels.push_back(row.folder ? row.name : PlaylistListDrop::DisplayName(row.name, row.favorite));
  }
  return labels;
}

}  // namespace PlaylistListKeyboard

#endif
