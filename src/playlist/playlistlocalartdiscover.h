#ifndef STRAWBERRY_PLAYLISTLOCALARTDISCOVER_H
#define STRAWBERRY_PLAYLISTLOCALARTDISCOVER_H

#include "core/song.h"

#include <string>

namespace PlaylistLocalArtDiscover {

inline bool ShouldWriteSidecar(const Song &row, const std::string &discovered) {
  if (discovered.empty() || row.id() > 0 || !row.art_manual().empty()) {
    return false;
  }
  return row.source() == Song::Source::LocalFile || row.source() == Song::Source::CDDA || row.source() == Song::Source::Device;
}

inline bool ShouldWriteSidecar(const Song &row, const Song &playing, const std::string &discovered) {
  return row.url() == playing.url() && ShouldWriteSidecar(row, discovered);
}

inline void ApplySidecar(Song *row, const std::string &discovered) {
  if (row && ShouldWriteSidecar(*row, discovered)) {
    row->set_art_manual(discovered);
  }
}

}  // namespace PlaylistLocalArtDiscover

#endif  // STRAWBERRY_PLAYLISTLOCALARTDISCOVER_H
