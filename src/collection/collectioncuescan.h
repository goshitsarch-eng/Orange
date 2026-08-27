#ifndef STRAWBERRY_COLLECTIONCUESCAN_H
#define STRAWBERRY_COLLECTIONCUESCAN_H

#include "utilities/fileutils.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace CollectionCueScan {

enum class Change { None, Added, Changed, Deleted };

inline int64_t CueMtime(const std::string &cue_path) {
  if (cue_path.empty() || !FileUtils::Exists(cue_path)) {
    return 0;
  }
  const int64_t mtime = FileUtils::FileMtime(cue_path);
  return mtime > 0 ? mtime : 0;
}

inline int64_t EffectiveMtime(int64_t media_mtime, int64_t cue_mtime) { return std::max(media_mtime, cue_mtime); }

inline Change DetectCueChange(bool had_cue, const std::string &old_cue, const std::string &new_cue, int64_t new_cue_mtime) {
  const bool have_new = new_cue_mtime > 0 && !new_cue.empty();
  if (have_new && !had_cue) {
    return Change::Added;
  }
  if (have_new && had_cue && new_cue != old_cue) {
    return Change::Changed;
  }
  if (had_cue && !have_new) {
    return Change::Deleted;
  }
  return Change::None;
}

inline bool CueForcesRescan(Change change) { return change != Change::None; }

}  // namespace CollectionCueScan

#endif  // STRAWBERRY_COLLECTIONCUESCAN_H
