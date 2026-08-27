#ifndef STRAWBERRY_COLLECTIONUNAVAILABLERESTORE_H
#define STRAWBERRY_COLLECTIONUNAVAILABLERESTORE_H

#include "collection/collectioncuescan.h"
#include "collection/collectionrescanreason.h"
#include "core/song.h"

#include <cstdint>

namespace CollectionUnavailableRestore {

inline bool UnchangedOnDisk(const Song &existing, int64_t mtime, int64_t filesize) {
  return existing.is_valid() && mtime > 0 && existing.mtime() == mtime && (filesize < 0 || existing.filesize() == filesize);
}

inline bool CanRestoreWithoutRescan(const Song &existing, int64_t mtime, int64_t filesize, bool song_tracking, bool ebu_analysis,
                                    CollectionCueScan::Change cue) {
  return existing.unavailable() && UnchangedOnDisk(existing, mtime, filesize) && !CollectionCueScan::CueForcesRescan(cue) &&
         !CollectionRescanReason::NeedsAnalysisRescan(existing, song_tracking, ebu_analysis);
}

}  // namespace CollectionUnavailableRestore

#endif  // STRAWBERRY_COLLECTIONUNAVAILABLERESTORE_H
