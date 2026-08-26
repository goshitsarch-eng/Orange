#ifndef STRAWBERRY_ALBUMCOVERMANAGERSELECTION_H
#define STRAWBERRY_ALBUMCOVERMANAGERSELECTION_H

#include "utilities/strutils.h"

#include <string>
#include <vector>

namespace AlbumCoverManagerSelection {

inline bool PreferSelection(size_t selected) { return selected > 0; }

inline std::string StatusText(size_t albums, size_t with_cover, size_t selected) {
  const size_t missing = albums > with_cover ? albums - with_cover : 0;
  std::string text = std::to_string(albums) + " albums · " + std::to_string(with_cover) + " with artwork · " + std::to_string(missing) + " missing";
  if (selected > 0) {
    text += " · " + std::to_string(selected) + " selected";
  }
  return text;
}

inline int WrapIndex(int current, int count, int delta) {
  if (count <= 0) {
    return -1;
  }
  int next = current + delta;
  while (next < 0) {
    next += count;
  }
  return next % count;
}

inline int FlowDelta(int columns, int horizontal, int vertical) {
  const int cols = columns > 0 ? columns : 1;
  return horizontal + vertical * cols;
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

}  // namespace AlbumCoverManagerSelection

#endif
