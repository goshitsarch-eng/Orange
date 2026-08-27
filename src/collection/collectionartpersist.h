#ifndef STRAWBERRY_COLLECTIONARTPERSIST_H
#define STRAWBERRY_COLLECTIONARTPERSIST_H

#include "core/song.h"

#include <string>

namespace CollectionArtPersist {

inline bool ShouldApplyAutomaticArt(const Song &existing, const std::string &candidate) {
  return !existing.art_unset() && existing.art_automatic().empty() && !candidate.empty();
}

inline std::string ArtAutomaticForUpdate(const Song &existing, const std::string &scanned) {
  if (existing.art_unset()) {
    return {};
  }
  return scanned.empty() ? existing.art_automatic() : scanned;
}

}  // namespace CollectionArtPersist

#endif  // STRAWBERRY_COLLECTIONARTPERSIST_H
