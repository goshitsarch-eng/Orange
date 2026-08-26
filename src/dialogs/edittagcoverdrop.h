#ifndef STRAWBERRY_EDITTAGCOVERDROP_H
#define STRAWBERRY_EDITTAGCOVERDROP_H

#include "constants/filefilterconstants.h"

#include <string>
#include <vector>

namespace EditTagCoverDrop {

inline std::vector<std::string> SplitUris(const std::string &text) {
  std::vector<std::string> out;
  std::string current;
  for (char ch : text) {
    if (ch == '\n') {
      if (!current.empty() && current.back() == '\r') {
        current.pop_back();
      }
      if (!current.empty()) {
        out.push_back(current);
      }
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty() && current.back() == '\r') {
    current.pop_back();
  }
  if (!current.empty()) {
    out.push_back(current);
  }
  return out;
}

inline std::string PathFromUriOrPath(const std::string &value) {
  if (value.rfind("file://", 0) == 0) {
    std::string path = value.substr(7);
    if (!path.empty() && path[0] != '/') {
      const std::string::size_type slash = path.find('/');
      path = slash == std::string::npos ? std::string() : path.substr(slash);
    }
    return path;
  }
  return value;
}

inline bool CanAcceptPath(const std::string &path) {
  return FileFilterConstants::PathMatchesGlobs(path, FileFilterConstants::kLoadImages);
}

inline std::string FirstImagePath(const std::string &text) {
  for (const std::string &part : SplitUris(text)) {
    const std::string path = PathFromUriOrPath(part);
    if (CanAcceptPath(path) || CanAcceptPath(part)) {
      return path.empty() ? part : path;
    }
  }
  return {};
}

}  // namespace EditTagCoverDrop

#endif
