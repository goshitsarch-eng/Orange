#ifndef STRAWBERRY_PLAYLISTMETADATAUPDATE_H
#define STRAWBERRY_PLAYLISTMETADATAUPDATE_H

#include "core/song.h"

namespace PlaylistMetadataUpdate {

// Qt Playlist::UpdateItems: replace a row when the URL matches and tags were only a preload stub.
inline bool ShouldReplace(const Song &existing, const Song &incoming) {
  if (existing.url().empty() || existing.url() != incoming.url()) {
    return false;
  }
  return existing.filetype() == Song::FileType::Unknown || existing.filetype() == Song::FileType::Stream ||
         existing.filetype() == Song::FileType::CDDA || !existing.init_from_file();
}

}  // namespace PlaylistMetadataUpdate

#endif
