#ifndef STRAWBERRY_PLAYERINTRO_H
#define STRAWBERRY_PLAYERINTRO_H

#include "core/playerrepeat.h"
#include "core/song.h"
#include "core/songsegment.h"
#include "engine/enginebase.h"
#include "playlist/playlist.h"
#include "playlist/playlistmanager.h"

#include <algorithm>
#include <cstdint>

namespace PlayerIntro {

inline bool Active(const Playlist *playlist) { return playlist && PlayerRepeat::IsIntro(playlist->repeat_mode()); }

inline bool Active(PlaylistManager *manager) { return manager && Active(manager->active()); }

inline int64_t EffectiveEndNanosec(const Song &song, bool intro) {
  const int64_t song_end = SongSegment::EffectiveEndNanosec(song);
  if (!intro) {
    return song_end;
  }
  const int64_t intro_end = song.beginning_nanosec() + PlayerRepeat::kIntroNanosec;
  if (song_end > 0) {
    return std::min(song_end, intro_end);
  }
  return intro_end;
}

inline bool HasForcedEnd(const Song &song, bool intro) { return intro || SongSegment::HasForcedEnd(song); }

inline int AdvanceFlags() { return EngineBase::Intro; }

}  // namespace PlayerIntro

#endif  // STRAWBERRY_PLAYERINTRO_H
