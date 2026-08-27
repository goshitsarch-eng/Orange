#ifndef STRAWBERRY_TRAYPROGRESSOVERLAY_H
#define STRAWBERRY_TRAYPROGRESSOVERLAY_H

#include <algorithm>
#include <string>

namespace TrayProgressOverlay {

inline int Decade(int percentage) {
  const int clamped = std::max(0, std::min(100, percentage));
  return (clamped / 10) * 10;
}

inline std::string IconName(int percentage, bool enabled, bool playing) {
  if (!enabled || !playing || percentage <= 0) {
    return {};
  }
  return "strawberry-progress-" + std::to_string(Decade(percentage));
}

}  // namespace TrayProgressOverlay

#endif
