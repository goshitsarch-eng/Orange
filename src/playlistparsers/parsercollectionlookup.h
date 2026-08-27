#ifndef STRAWBERRY_PARSERCOLLECTIONLOOKUP_H
#define STRAWBERRY_PARSERCOLLECTIONLOOKUP_H

#include "collection/collectionbackend.h"
#include "core/song.h"

namespace ParserCollectionLookup {

inline Song Resolve(const Song &stub, CollectionBackend *backend, int64_t beginning_nanosec = -1) {
  if (!backend || stub.url().empty()) {
    return stub;
  }
  const Song found = beginning_nanosec >= 0 ? backend->SongByUrl(stub.url(), beginning_nanosec) : backend->SongByUrl(stub.url());
  if (found.id() > 0 || found.is_valid()) {
    return found;
  }
  return stub;
}

}  // namespace ParserCollectionLookup

#endif  // STRAWBERRY_PARSERCOLLECTIONLOOKUP_H
