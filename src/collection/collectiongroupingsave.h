#ifndef STRAWBERRY_COLLECTIONGROUPINGSAVE_H
#define STRAWBERRY_COLLECTIONGROUPINGSAVE_H

#include "collection/collectionfiltermenu.h"
#include "collection/collectiongrouping.h"
#include "utilities/strutils.h"

#include <string>
#include <utility>
#include <vector>

namespace CollectionGroupingSave {

inline const char *DialogTitle() { return "Grouping Name"; }
inline const char *DialogPrompt() { return "Grouping name:"; }
inline const char *SaveLabel() { return "Save current grouping"; }
inline const char *ManageLabel() { return "Manage saved groupings"; }

inline std::string TrimmedName(const std::string &name) { return StrUtils::Trim(name); }

inline bool AcceptName(const std::string &name) { return !TrimmedName(name).empty(); }

inline bool Save(const std::string &name, const CollectionGrouping::Grouping &grouping) {
  const std::string trimmed = TrimmedName(name);
  if (trimmed.empty()) {
    return false;
  }
  CollectionGrouping::AddSaved(trimmed, grouping);
  return true;
}

inline int MenuCheckIndex(const CollectionGrouping::Grouping &current, const std::vector<CollectionFilterMenu::Preset> &presets,
                          const std::vector<std::pair<std::string, CollectionGrouping::Grouping>> &saved) {
  const int preset = CollectionFilterMenu::MatchingPresetIndex(current, presets);
  if (preset >= 0) {
    return preset;
  }
  for (size_t i = 0; i < saved.size(); ++i) {
    if (saved[i].second == current) {
      return static_cast<int>(presets.size()) + static_cast<int>(i);
    }
  }
  return -1;
}

inline std::string MenuStateKey(int check_index, int preset_count) {
  if (check_index < 0) {
    return "advanced";
  }
  if (check_index < preset_count) {
    return "p" + std::to_string(check_index);
  }
  return "s" + std::to_string(check_index - preset_count);
}

inline std::string MenuLabel(const std::string &label, bool checked) { return checked ? ("✓ " + label) : label; }

}  // namespace CollectionGroupingSave

#endif  // STRAWBERRY_COLLECTIONGROUPINGSAVE_H
