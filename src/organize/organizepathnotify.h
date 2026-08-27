#ifndef STRAWBERRY_ORGANIZEPATHNOTIFY_H
#define STRAWBERRY_ORGANIZEPATHNOTIFY_H

#include "collection/collectionbackend.h"
#include "core/song.h"

namespace OrganizePathNotify {

inline bool ShouldNotify(bool move, const Song &song, bool destination_is_collection) {
  return move && destination_is_collection && song.is_collection_song() && song.id() > 0;
}

inline bool DestinationIsCollection(const std::string &destination, CollectionBackend *backend) {
  if (!backend || destination.empty()) {
    return false;
  }
  for (const CollectionDirectory &directory : backend->Directories()) {
    if (destination == directory.path || destination.rfind(directory.path + "/", 0) == 0) {
      return true;
    }
  }
  return false;
}

inline int DirectoryIdForPath(const std::string &destination, CollectionBackend *backend) {
  if (!backend) {
    return -1;
  }
  int best = -1;
  size_t best_len = 0;
  for (const CollectionDirectory &directory : backend->Directories()) {
    if ((destination == directory.path || destination.rfind(directory.path + "/", 0) == 0) && directory.path.size() > best_len) {
      best = directory.id;
      best_len = directory.path.size();
    }
  }
  return best;
}

}  // namespace OrganizePathNotify

#endif  // STRAWBERRY_ORGANIZEPATHNOTIFY_H
