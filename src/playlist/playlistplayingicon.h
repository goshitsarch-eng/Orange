#ifndef STRAWBERRY_PLAYLISTPLAYINGICON_H
#define STRAWBERRY_PLAYLISTPLAYINGICON_H

namespace PlaylistPlayingIcon {

inline constexpr int kPixelSize = 12;

inline const char *Name(const bool paused) { return paused ? "media-playback-pause-symbolic" : "media-playback-start-symbolic"; }

inline bool ShowOnCurrentRow(const bool is_current) { return is_current; }

}  // namespace PlaylistPlayingIcon

#endif
