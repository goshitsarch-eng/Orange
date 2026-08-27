#ifndef STRAWBERRY_PLAYLISTSLOADEDGATE_H
#define STRAWBERRY_PLAYLISTSLOADEDGATE_H

#include "core/playerresume.h"

namespace PlaylistsLoadedGate {

// Qt MainWindow::CommandlineOptionsReceived stores options until AllPlaylistsLoaded.
inline bool ShouldDeferCommandline(bool playlists_loaded) { return !playlists_loaded; }

// Qt Player::Play sets play_requested_ when playlists are not loaded yet.
inline bool DeferPlay(bool playlists_loaded) { return !playlists_loaded; }

// Qt Player::PlaylistsLoaded: resume saved playing/paused, else honor a deferred Play().
inline bool ShouldResumeAfterLoad(bool resume_enabled, int saved_state) {
  return PlayerResume::ShouldResume(resume_enabled, saved_state);
}

inline bool ShouldHonorPlayRequest(bool resume_enabled, int saved_state, bool play_requested) {
  return play_requested && !ShouldResumeAfterLoad(resume_enabled, saved_state);
}

}  // namespace PlaylistsLoadedGate

#endif
