#ifndef STRAWBERRY_PLAYLISTLOOK_H
#define STRAWBERRY_PLAYLISTLOOK_H

#include <algorithm>
#include <string>

namespace PlaylistLook {

inline std::string AlternatingCss(bool enabled) {
  if (!enabled) {
    return {};
  }
  return ".playlist-row.playlist-alt { background-color: alpha(currentColor, 0.06); }";
}

inline std::string GlowCss(bool enabled) {
  if (!enabled) {
    return {};
  }
  return ".playlist-playing.playlist-glow { background-color: alpha(@accent_bg_color, 0.22); }";
}

inline std::string BarsCss(bool enabled, double progress) {
  if (!enabled) {
    return {};
  }
  const int pct = std::clamp(static_cast<int>(progress * 100.0), 0, 100);
  return ".playlist-playing.playlist-bars { background-image: linear-gradient(to right, alpha(@accent_bg_color, 0.35) " +
         std::to_string(pct) + "%, transparent " + std::to_string(pct) + "%); }";
}

inline std::string UnavailableCss() {
  return ".playlist-row.playlist-unavailable, .playlist-row.playlist-unavailable label { color: #c0c0c0; }";
}

inline std::string CombinedCss(bool alternating, bool glow, bool bars, double progress) {
  return AlternatingCss(alternating) + GlowCss(glow) + BarsCss(bars, progress) + UnavailableCss();
}

}  // namespace PlaylistLook

#endif
