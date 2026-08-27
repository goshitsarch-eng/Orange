#ifndef STRAWBERRY_SUBSONICSCROBBLESTATE_H
#define STRAWBERRY_SUBSONICSCROBBLESTATE_H

#include "core/song.h"

#include <string>

namespace SubsonicScrobbleState {

inline bool ServerSideEnabled(bool setting_enabled) { return setting_enabled; }

inline bool ShouldNowPlaying(const Song &song, bool server_side) {
  return server_side && song.source() == Song::Source::Subsonic && (!song.song_id().empty() || !song.url().empty());
}

inline bool ShouldSubmit(const Song &song, const std::string &playing_url, const std::string &playing_song_id) {
  if (song.source() != Song::Source::Subsonic) {
    return false;
  }
  if (!playing_song_id.empty() && song.song_id() == playing_song_id) {
    return true;
  }
  return !playing_url.empty() && song.url() == playing_url;
}

inline std::string TrackId(const Song &song) { return song.song_id().empty() ? song.url() : song.song_id(); }

// Qt SubsonicScrobbler::Scrobble uses settings submit_delay seconds as-is (no 5s Last.fm floor).
inline bool ShouldSubmitImmediately(int submit_delay_seconds, bool already_submitted) {
  return !already_submitted && submit_delay_seconds <= 0;
}

inline bool ShouldStartSubmitTimer(int submit_delay_seconds, bool already_submitted, bool timer_active) {
  return !already_submitted && submit_delay_seconds > 0 && !timer_active;
}

inline int DelaySeconds(int submit_delay_seconds) { return submit_delay_seconds > 0 ? submit_delay_seconds : 0; }

}  // namespace SubsonicScrobbleState

#endif  // STRAWBERRY_SUBSONICSCROBBLESTATE_H
