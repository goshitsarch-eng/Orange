#ifndef STRAWBERRY_TRAYICONCOMPOSITE_H
#define STRAWBERRY_TRAYICONCOMPOSITE_H

#include "systemtrayicon/trayprogressoverlay.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace TrayIconComposite {

enum class Playback { Stopped, Playing, Paused };

inline constexpr double kHalfPi = 1.57079632679489661923;

inline Playback StateFrom(bool playing, bool paused) {
  if (paused) {
    return Playback::Paused;
  }
  if (playing) {
    return Playback::Playing;
  }
  return Playback::Stopped;
}

inline const char *BaseIconName() { return "strawberry"; }

inline const char *BadgeIconName(Playback state) {
  switch (state) {
    case Playback::Playing:
      return "media-playback-start";
    case Playback::Paused:
      return "media-playback-pause";
    case Playback::Stopped:
      break;
  }
  return "";
}

inline bool ShowsProgress(int percentage, bool enabled, Playback state) {
  return enabled && state != Playback::Stopped && percentage > 0;
}

inline std::string OverlayName(int percentage, bool progress_enabled, Playback state) {
  if (ShowsProgress(percentage, progress_enabled, state)) {
    return TrayProgressOverlay::IconName(percentage, true, true);
  }
  return BadgeIconName(state);
}

inline double CoverAngle(int percentage) {
  const int clamped = std::max(0, std::min(100, percentage));
  return static_cast<double>(100 - clamped) / 100.0 * kHalfPi;
}

inline double CoverLength(int width, int height) { return std::sqrt(static_cast<double>(width * width + height * height)); }

struct Point {
  int x = 0;
  int y = 0;
};

inline Point CoverLineEnd(int width, int height, int percentage) {
  const double length = CoverLength(width, height);
  const double angle = CoverAngle(percentage);
  return {static_cast<int>(length * std::sin(angle)), static_cast<int>(length * std::cos(angle))};
}

inline bool CoverIncludesBottomRight(int percentage) { return percentage > 50; }

inline int BadgeHeight(int icon_height) { return icon_height / 2; }

inline Point BadgeTopLeft(int icon_width, int badge_width) { return {icon_width - badge_width, 0}; }

}  // namespace TrayIconComposite

#endif  // STRAWBERRY_TRAYICONCOMPOSITE_H
