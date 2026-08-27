#ifndef STRAWBERRY_PLAYLISTSTREAMSTATE_H
#define STRAWBERRY_PLAYLISTSTREAMSTATE_H

#include "core/song.h"

namespace PlaylistStreamState {

inline void ClearResolved(Song *song) {
  if (!song) {
    return;
  }
  song->set_stream_url({});
}

inline void ClearForRowChange(Song *current, Song *next) {
  ClearResolved(current);
  ClearResolved(next);
}

}  // namespace PlaylistStreamState

#endif  // STRAWBERRY_PLAYLISTSTREAMSTATE_H
