#ifndef STRAWBERRY_FILEVIEWDRAG_H
#define STRAWBERRY_FILEVIEWDRAG_H

#include "utilities/fileutils.h"

#include <string>
#include <vector>

namespace FileViewDrag {

inline std::string DragPayload(const std::vector<std::string> &paths) {
  std::string text;
  for (const std::string &path : paths) {
    if (path.empty() || FileUtils::IsDirectory(path)) {
      continue;
    }
    if (!text.empty()) {
      text += "\n";
    }
    text += FileUtils::UriFromPath(path);
  }
  return text;
}

}  // namespace FileViewDrag

#endif
