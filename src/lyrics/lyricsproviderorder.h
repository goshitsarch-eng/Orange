#ifndef STRAWBERRY_LYRICSPROVIDERORDER_H
#define STRAWBERRY_LYRICSPROVIDERORDER_H

#include "utilities/strutils.h"

#include <string>
#include <vector>

namespace LyricsProviderOrder {

inline std::vector<std::string> Parse(const std::string &csv) {
  std::vector<std::string> names;
  for (const std::string &part : StrUtils::Split(csv, ',')) {
    const std::string name = StrUtils::Trim(part);
    if (!name.empty()) {
      names.push_back(name);
    }
  }
  return names;
}

inline std::string Join(const std::vector<std::string> &names) { return StrUtils::Join(names, ","); }

inline int Rank(const std::vector<std::string> &order, const std::string &name, int fallback) {
  for (size_t i = 0; i < order.size(); ++i) {
    if (order[i] == name) {
      return static_cast<int>(i);
    }
  }
  return fallback;
}

inline std::vector<std::string> Move(std::vector<std::string> names, int index, int delta) {
  const int dest = index + delta;
  if (index < 0 || dest < 0 || dest >= static_cast<int>(names.size())) {
    return names;
  }
  std::swap(names[static_cast<size_t>(index)], names[static_cast<size_t>(dest)]);
  return names;
}

}  // namespace LyricsProviderOrder

#endif
