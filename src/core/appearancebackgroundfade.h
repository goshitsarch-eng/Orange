#ifndef STRAWBERRY_APPEARANCEBACKGROUNDFADE_H
#define STRAWBERRY_APPEARANCEBACKGROUNDFADE_H

#include <string>

namespace AppearanceBackgroundFade {

// Qt PlaylistView fade_animation_ is a 1000 ms QTimeLine running Backward (1 → 0).
inline constexpr int kDurationMs = 1000;
inline constexpr int kTickMs = 50;

inline const char *kPreviousSelector() { return ".strawberry-playlist-previous-background"; }

inline double PreviousOpacity(int elapsed_ms) {
  if (elapsed_ms <= 0) {
    return 1.0;
  }
  if (elapsed_ms >= kDurationMs) {
    return 0.0;
  }
  return 1.0 - static_cast<double>(elapsed_ms) / static_cast<double>(kDurationMs);
}

inline double CurrentOpacity(int elapsed_ms) { return 1.0 - PreviousOpacity(elapsed_ms); }

inline bool ShouldReplace(const std::string &current_key, const std::string &new_key) { return current_key != new_key; }

// Qt only starts the timeline when the playlist view is visible and a prior image exists.
inline bool ShouldAnimate(bool visible, bool had_background) { return visible && had_background; }

inline bool ShouldClearPrevious(double previous_opacity) { return previous_opacity <= 0.0; }

inline std::string RewriteSelector(const std::string &css, const std::string &from, const std::string &to) {
  if (from.empty() || from == to) {
    return css;
  }
  std::string out = css;
  size_t pos = 0;
  while ((pos = out.find(from, pos)) != std::string::npos) {
    out.replace(pos, from.size(), to);
    pos += to.size();
  }
  return out;
}

}  // namespace AppearanceBackgroundFade

#endif
