#ifndef STRAWBERRY_OSDPRETTYFADE_H
#define STRAWBERRY_OSDPRETTYFADE_H

#include <algorithm>

namespace OSDPrettyFade {

inline constexpr int kDurationMs = 300;
inline constexpr int kTickMs = 16;

inline double OpacityAt(const int elapsed_ms, const int duration_ms, const bool fading_in) {
  if (duration_ms <= 0) {
    return fading_in ? 1.0 : 0.0;
  }
  const double t = std::clamp(static_cast<double>(elapsed_ms) / static_cast<double>(duration_ms), 0.0, 1.0);
  return fading_in ? t : 1.0 - t;
}

inline bool Finished(const int elapsed_ms, const int duration_ms) { return elapsed_ms >= duration_ms; }

}  // namespace OSDPrettyFade

#endif
