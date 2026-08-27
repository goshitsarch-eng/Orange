#ifndef STRAWBERRY_COLLECTIONFINGERPRINTMATCH_H
#define STRAWBERRY_COLLECTIONFINGERPRINTMATCH_H

#include "core/song.h"
#include "utilities/fileutils.h"

#include <string>

namespace CollectionFingerprintMatch {

inline bool IsUsable(const std::string &fingerprint) { return !fingerprint.empty() && fingerprint != "NONE"; }

inline bool OldPathGone(const std::string &url) {
  if (url.empty()) {
    return true;
  }
  const std::string path = FileUtils::PathFromUri(url);
  return path.empty() || !FileUtils::Exists(path);
}

inline const Song *PickMovedMatch(const SongList &candidates, const std::string &new_url) {
  for (const Song &song : candidates) {
    if (!song.is_valid() || song.url() == new_url) {
      continue;
    }
    if (OldPathGone(song.url())) {
      return &song;
    }
  }
  return nullptr;
}

}  // namespace CollectionFingerprintMatch

#endif  // STRAWBERRY_COLLECTIONFINGERPRINTMATCH_H
