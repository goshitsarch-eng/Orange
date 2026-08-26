#ifndef COLLECTIONAUTOOPEN_H
#define COLLECTIONAUTOOPEN_H

#include "config.h"

#include "collection/collectionitem.h"
#include "collection/collectiontree.h"
#include "constants/collectionsettings.h"
#include "core/settings.h"

#include <set>
#include <string>
#include <vector>

namespace CollectionAutoOpen {

inline constexpr int kRowsToShow = 50;

inline bool LoadAutoOpen() {
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  return settings.BoolValue(CollectionSettings::kAutoOpen, CollectionSettings::kDefaultAutoOpen);
}

inline int ChildCount(const CollectionItem *item) { return item ? static_cast<int>(item->children.size()) : 0; }

inline bool ShouldDrillInto(const bool auto_open, const CollectionItem *item) {
  return auto_open && CollectionTree::IsExpandable(item) && ChildCount(item) == 1;
}

inline const CollectionItem *SoleChild(const CollectionItem *item) {
  if (ChildCount(item) != 1) {
    return nullptr;
  }
  return item->children.front().get();
}

inline std::vector<std::string> DrillKeys(const bool auto_open, const CollectionItem *item) {
  std::vector<std::string> keys;
  const CollectionItem *current = item;
  while (ShouldDrillInto(auto_open, current)) {
    current = SoleChild(current);
    if (!current) {
      break;
    }
    if (CollectionTree::IsExpandable(current)) {
      keys.push_back(CollectionTree::Key(current));
    }
  }
  return keys;
}

inline void ApplyDrill(std::set<std::string> *expanded, const bool auto_open, const CollectionItem *item) {
  if (!expanded) {
    return;
  }
  for (const std::string &key : DrillKeys(auto_open, item)) {
    expanded->insert(key);
  }
}

inline bool RecursivelyExpandItem(const CollectionItem *item, int *count, std::vector<std::string> *keys, const int rows_to_show = kRowsToShow) {
  if (!item || !count || !keys) {
    return true;
  }
  if (item->type != CollectionItem::Type::Root && !CollectionTree::IsExpandable(item)) {
    return true;
  }
  const int children = ChildCount(item);
  if (*count + children > rows_to_show) {
    return false;
  }
  if (CollectionTree::IsExpandable(item)) {
    keys->push_back(CollectionTree::Key(item));
  }
  *count += children;
  for (const auto &child : item->children) {
    if (!RecursivelyExpandItem(child.get(), count, keys, rows_to_show)) {
      return false;
    }
  }
  return true;
}

inline std::vector<std::string> RecursivelyExpandKeys(const CollectionItem *root, const bool auto_open, const int rows_to_show = kRowsToShow) {
  std::vector<std::string> keys;
  if (!auto_open || !root) {
    return keys;
  }
  int count = ChildCount(root);
  RecursivelyExpandItem(root, &count, &keys, rows_to_show);
  return keys;
}

}  // namespace CollectionAutoOpen

#endif  // COLLECTIONAUTOOPEN_H
