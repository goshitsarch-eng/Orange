#ifndef STRAWBERRY_COLLECTIONTREELEFT_H
#define STRAWBERRY_COLLECTIONTREELEFT_H

#include "collection/collectionitem.h"

namespace CollectionTreeLeft {

enum class Action { None, CollapseCurrent, SelectParentAndCollapse };

// Qt AutoExpandingTreeView::keyPressEvent Left, then QTreeView Left.
// Expanded containers collapse in place. Leaves and collapsed rows jump to the parent and collapse it.
inline bool IsRootRow(const CollectionItem *item) {
  return !item || !item->parent || item->parent->type == CollectionItem::Type::Root;
}

inline const CollectionItem *SelectableParent(const CollectionItem *item) {
  if (IsRootRow(item)) {
    return nullptr;
  }
  return item->parent;
}

inline Action FromState(bool is_root, bool expanded, bool has_children) {
  if (expanded && has_children) {
    return Action::CollapseCurrent;
  }
  if (!is_root) {
    return Action::SelectParentAndCollapse;
  }
  return Action::None;
}

inline Action FromItem(const CollectionItem *item, bool expanded) {
  if (!item) {
    return Action::None;
  }
  return FromState(IsRootRow(item), expanded, !item->children.empty());
}

inline const CollectionItem *FocusItem(const CollectionItem *item, Action action) {
  if (action == Action::SelectParentAndCollapse) {
    return SelectableParent(item);
  }
  return item;
}

inline const CollectionItem *CollapseItem(const CollectionItem *item, Action action, bool parent_expanded) {
  if (action == Action::CollapseCurrent) {
    return item;
  }
  if (action == Action::SelectParentAndCollapse && parent_expanded) {
    return SelectableParent(item);
  }
  return nullptr;
}

}  // namespace CollectionTreeLeft

#endif
