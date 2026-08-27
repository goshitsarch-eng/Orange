#ifndef STRAWBERRY_PLAYLISTCOVERPERSIST_H
#define STRAWBERRY_PLAYLISTCOVERPERSIST_H

#include "core/song.h"

#include <string>

namespace PlaylistCoverPersist {

inline bool ShouldPersistLocalArt(const Song &row, const Song &playing, const std::string &art_manual) {
  if (art_manual.empty() || row.url() != playing.url() || row.id() > 0) {
    return false;
  }
  return row.source() == Song::Source::LocalFile || row.source() == Song::Source::CDDA || row.source() == Song::Source::Device;
}

}  // namespace PlaylistCoverPersist

#endif  // STRAWBERRY_PLAYLISTCOVERPERSIST_H
