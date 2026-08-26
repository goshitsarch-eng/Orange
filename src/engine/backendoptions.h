#ifndef STRAWBERRY_BACKENDOPTIONS_H
#define STRAWBERRY_BACKENDOPTIONS_H

#include <algorithm>
#include <string>

namespace BackendOptions {

inline const char *PlaybinFactory(bool playbin3) { return playbin3 ? "playbin3" : "playbin"; }

inline bool SameAlbum(const std::string &current_album, const std::string &next_album) {
  return !current_album.empty() && current_album == next_album;
}

inline bool AllowAutoCrossfade(bool autocrossfade, bool no_crossfade_same_album, const std::string &current_album,
                               const std::string &next_album) {
  if (!autocrossfade) {
    return false;
  }
  if (no_crossfade_same_album && SameAlbum(current_album, next_album)) {
    return false;
  }
  return true;
}

inline int FadeDurationMs(bool enabled, int duration_ms, int fallback_ms) {
  if (!enabled) {
    return 0;
  }
  return std::max(50, duration_ms > 0 ? duration_ms : fallback_ms);
}

}  // namespace BackendOptions

#endif
