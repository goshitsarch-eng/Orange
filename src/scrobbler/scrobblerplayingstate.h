#ifndef STRAWBERRY_SCROBBLERPLAYINGSTATE_H
#define STRAWBERRY_SCROBBLERPLAYINGSTATE_H

#include "core/song.h"

#include <cstdint>

// Now-playing / radio-scrobble rules matching Qt LastFMScrobbler (scrobbler/lastfmscrobbler.cpp).
namespace ScrobblerPlayingState {

inline constexpr int64_t kRadioScrobbleSeconds = 30;

inline bool SameAsPlaying(const Song &song, const Song &playing) {
  return song.id() == playing.id() && song.url() == playing.url() && song.is_metadata_good();
}

inline int64_t ElapsedSeconds(uint64_t started_at, uint64_t now) {
  if (started_at == 0 || now < started_at) {
    return 0;
  }
  return static_cast<int64_t>(now - started_at);
}

inline bool ShouldScrobbleRadioPrev(const Song &playing, bool already_scrobbled, int64_t elapsed_seconds) {
  return !already_scrobbled && playing.is_metadata_good() && playing.is_radio() && elapsed_seconds > kRadioScrobbleSeconds;
}

inline uint64_t TimestampOrNow(uint64_t started_at, uint64_t now) { return started_at != 0 ? started_at : now; }

}  // namespace ScrobblerPlayingState

#endif
