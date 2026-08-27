#ifndef STRAWBERRY_PLAYLISTLISTLOOK_H
#define STRAWBERRY_PLAYLISTLISTLOOK_H

#include "playlist/playlistlistdrop.h"

#include <string>

namespace PlaylistListLook {

enum class Playback { Stopped, Playing, Paused };

inline constexpr int kDragHoverTimeoutMs = 500;

inline const char *PlaybackIconName(bool is_active, Playback playback) {
  if (!is_active || playback == Playback::Stopped) {
    return nullptr;
  }
  return playback == Playback::Paused ? "media-playback-pause-symbolic" : "media-playback-start-symbolic";
}

inline bool IsActiveName(const std::string &name, const std::string &active_name) { return !name.empty() && name == active_name; }

inline bool ShouldStartDragHover(const std::string &payload) { return PlaylistListDrop::IsPlaylistRows(payload); }

inline bool ShouldRestartDragHover(const std::string &hovered, const std::string &current) {
  return !hovered.empty() && hovered != current;
}

inline bool DragHoverShouldActivate(int elapsed_ms) { return elapsed_ms >= kDragHoverTimeoutMs; }

inline bool ShouldAcceptPlaylistRowsDrop(int target_playlist_id, int active_playlist_id) {
  return target_playlist_id >= 0 && target_playlist_id != active_playlist_id;
}

inline bool ShouldShowEmptyHint(size_t row_count) { return row_count == 0; }

inline const char *EmptyHint() {
  return "You can favorite playlists by clicking the star icon next to a playlist name\n\nFavorited playlists will be saved here";
}

}  // namespace PlaylistListLook

#endif
