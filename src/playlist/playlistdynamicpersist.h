#ifndef STRAWBERRY_PLAYLISTDYNAMICPERSIST_H
#define STRAWBERRY_PLAYLISTDYNAMICPERSIST_H

#include "smartplaylists/playlistgenerator.h"
#include "smartplaylists/smartplaylist.h"

#include <string>

namespace DynamicPlaylistPersist {

inline int TypeFor(bool is_dynamic) {
  return is_dynamic ? static_cast<int>(PlaylistGenerator::Type::Query) : static_cast<int>(PlaylistGenerator::Type::None);
}

inline bool IsDynamic(int type) { return type == static_cast<int>(PlaylistGenerator::Type::Query); }

inline std::string Encode(const SmartPlaylistSearch &search) { return search.Serialize(); }

inline SmartPlaylistSearch Decode(const std::string &blob) {
  SmartPlaylistSearch search;
  if (!blob.empty()) {
    SmartPlaylistSearch::Parse(blob, &search);
  }
  return search;
}

inline const char *DefaultBackend() { return "songs"; }

}  // namespace DynamicPlaylistPersist

#endif  // STRAWBERRY_PLAYLISTDYNAMICPERSIST_H
