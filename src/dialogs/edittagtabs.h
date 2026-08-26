#ifndef STRAWBERRY_EDITTAGTABS_H
#define STRAWBERRY_EDITTAGTABS_H

#include <string>

namespace EditTagTabs {

inline constexpr const char *kNames[] = {"Summary", "Tags", "Lyrics", "Statistics"};
inline constexpr int kCount = 4;

inline int ClampIndex(int index, int count = kCount) {
  if (count <= 0) {
    return 0;
  }
  if (index < 0) {
    return 0;
  }
  if (index >= count) {
    return count - 1;
  }
  return index;
}

inline const char *Name(int index) { return kNames[ClampIndex(index)]; }

inline int IndexFromName(const std::string &name) {
  for (int i = 0; i < kCount; ++i) {
    if (name == kNames[i]) {
      return i;
    }
  }
  return 0;
}

}  // namespace EditTagTabs

#endif
