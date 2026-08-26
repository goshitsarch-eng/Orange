#ifndef STRAWBERRY_COLLECTIONSTATS_H
#define STRAWBERRY_COLLECTIONSTATS_H

#include "core/song.h"
#include "utilities/fileutils.h"

#include <string>

namespace CollectionStats {

inline bool ShouldWriteStatistics(const Song &song) {
  if (!song.is_local_file()) {
    return false;
  }
  return !FileUtils::PathFromUri(song.url()).empty();
}

inline int SongsToWrite(const SongList &songs) {
  int count = 0;
  for (const Song &song : songs) {
    if (ShouldWriteStatistics(song)) {
      ++count;
    }
  }
  return count;
}

inline const char *ConfirmTitle() { return "Write all playcounts and ratings to files"; }

inline const char *ConfirmText() {
  return "Are you sure you want to write song playcounts and ratings to file for all songs in your collection?";
}

inline const char *SaveNowLabel() { return "Save playcounts and ratings to files now"; }

inline const char *TaskName() { return "Saving playcounts and ratings"; }

inline const char *CacheInUseTitle() { return "Current disk cache in use:"; }

inline const char *ClearCacheLabel() { return "Clear Disk Cache"; }

}  // namespace CollectionStats

#endif
