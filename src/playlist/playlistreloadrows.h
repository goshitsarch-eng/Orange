#ifndef STRAWBERRY_PLAYLISTRELOADROWS_H
#define STRAWBERRY_PLAYLISTRELOADROWS_H

#include "playlist/playlistbehaviour.h"
#include "core/song.h"

namespace PlaylistReloadRows {

inline bool ShouldReload(const Song &before, bool exists_now) {
  return PlaylistBehaviour::IsLocalMedia(before) && before.unavailable() && exists_now;
}

}  // namespace PlaylistReloadRows

#endif  // STRAWBERRY_PLAYLISTRELOADROWS_H
