#ifndef STRAWBERRY_FILEVIEWHIDDEN_H
#define STRAWBERRY_FILEVIEWHIDDEN_H

#include <string>

namespace FileViewHidden {

inline bool IsHiddenEntry(const std::string &name) { return !name.empty() && name[0] == '.'; }

inline bool ShouldIncludeEntry(const std::string &name, bool show_hidden) {
  return !name.empty() && (show_hidden || !IsHiddenEntry(name));
}

}  // namespace FileViewHidden

#endif
