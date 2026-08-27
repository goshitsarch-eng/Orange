#ifndef STRAWBERRY_PLAYCOUNTINCREMENT_H
#define STRAWBERRY_PLAYCOUNTINCREMENT_H

#include "core/song.h"

namespace PlayCountIncrement {

inline bool ShouldIncrementOnTrackEnd(const Song &song) { return song.is_collection_song() && song.id() > 0; }

inline bool ShouldScrobbleSeparately(bool increment_playcount, bool should_scrobble) {
  return should_scrobble || increment_playcount;
}

}  // namespace PlayCountIncrement

#endif  // STRAWBERRY_PLAYCOUNTINCREMENT_H
