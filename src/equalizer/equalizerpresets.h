#ifndef STRAWBERRY_EQUALIZERPRESETS_H
#define STRAWBERRY_EQUALIZERPRESETS_H

#include "equalizer/equalizerpersist.h"

#include <string>
#include <vector>

namespace EqualizerPresets {

inline std::vector<std::string> AfterDelete(const std::vector<std::string> &presets, const std::string &deleted) {
  std::vector<std::string> remaining;
  remaining.reserve(presets.size());
  for (const std::string &name : presets) {
    if (name != deleted) {
      remaining.push_back(name);
    }
  }
  return remaining;
}

inline std::string NextSelected(const std::vector<std::string> &remaining, const std::string &deleted, const std::string &current) {
  if (current != deleted) {
    for (const std::string &name : remaining) {
      if (name == current) {
        return current;
      }
    }
  }
  return remaining.empty() ? std::string(EqualizerPersist::kDefaultPreset) : remaining.front();
}

inline std::string ConfirmDeleteMessage(const std::string &name) {
  return "Are you sure you want to delete the \"" + name + "\" preset?";
}

inline int IndexOf(const std::vector<std::string> &names, const std::string &name) {
  for (size_t i = 0; i < names.size(); ++i) {
    if (names[i] == name) {
      return static_cast<int>(i);
    }
  }
  return 0;
}

}  // namespace EqualizerPresets

#endif
