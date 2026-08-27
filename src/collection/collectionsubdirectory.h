#ifndef STRAWBERRY_COLLECTIONSUBDIRECTORY_H
#define STRAWBERRY_COLLECTIONSUBDIRECTORY_H

#include "utilities/fileutils.h"

#include <cstdint>
#include <string>
#include <vector>

struct CollectionSubdirectory {
  int directory_id = -1;
  std::string path;
  int64_t mtime = -1;
};

namespace CollectionSubdirectoryScan {

inline bool ShouldSkip(int64_t stored_mtime, int64_t path_mtime, bool force = false) {
  return !force && stored_mtime > 0 && path_mtime > 0 && stored_mtime == path_mtime;
}

inline std::string ImmediateParent(const std::string &path) { return FileUtils::DirName(path); }

inline int64_t StoredMtime(const std::vector<CollectionSubdirectory> &subdirs, const std::string &path) {
  for (const CollectionSubdirectory &subdir : subdirs) {
    if (subdir.path == path) {
      return subdir.mtime;
    }
  }
  return -1;
}

}  // namespace CollectionSubdirectoryScan

#endif  // STRAWBERRY_COLLECTIONSUBDIRECTORY_H
