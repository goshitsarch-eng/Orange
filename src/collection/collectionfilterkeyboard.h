#ifndef COLLECTIONFILTERKEYBOARD_H
#define COLLECTIONFILTERKEYBOARD_H

#include "collection/collectionitem.h"
#include "collection/collectionkeyboard.h"
#include "widgets/listboxkeyboard.h"

namespace CollectionFilterKeyboard {

enum class Action { None, Activate, MoveUp, MoveDown, Clear, FocusFilter };

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

inline bool CanActivate(const CollectionItem *item) {
  return item && item->type != CollectionItem::Type::Divider && item->type != CollectionItem::Type::Root &&
         item->type != CollectionItem::Type::LoadingIndicator;
}

inline const CollectionItem *FirstActivatable(const CollectionItem *root) {
  if (!root) {
    return nullptr;
  }
  for (const auto &child : root->children) {
    if (CanActivate(child.get())) {
      return child.get();
    }
  }
  return nullptr;
}

inline CollectionKeyboard::Action SearchMoveAction(const Action action) {
  if (action == Action::MoveUp) {
    return CollectionKeyboard::Action::MoveUp;
  }
  if (action == Action::MoveDown) {
    return CollectionKeyboard::Action::MoveDown;
  }
  return CollectionKeyboard::Action::None;
}

}  // namespace CollectionFilterKeyboard

#endif  // COLLECTIONFILTERKEYBOARD_H
