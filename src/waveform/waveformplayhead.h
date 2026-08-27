#ifndef STRAWBERRY_WAVEFORMPLAYHEAD_H
#define STRAWBERRY_WAVEFORMPLAYHEAD_H

#include <algorithm>
#include <cstdint>

namespace WaveformPlayhead {

// Qt WaveformProxyStyle: 1px cursor, played region at 55% alpha, 1s mode fade.
inline constexpr int kCursorWidth = 1;
inline constexpr float kPlayedAlpha = 0.55F;
inline constexpr int kFadeDurationMs = 1000;

inline double Progress(int64_t position_nanosec, int64_t length_nanosec) {
  if (length_nanosec <= 0 || position_nanosec <= 0) {
    return 0.0;
  }
  if (position_nanosec >= length_nanosec) {
    return 1.0;
  }
  return static_cast<double>(position_nanosec) / static_cast<double>(length_nanosec);
}

inline int SplitX(int64_t position_nanosec, int64_t length_nanosec, int width) {
  if (width <= 0 || length_nanosec <= 0) {
    return 0;
  }
  const int64_t x = position_nanosec * static_cast<int64_t>(width) / length_nanosec;
  return static_cast<int>(std::clamp(x, static_cast<int64_t>(0), static_cast<int64_t>(width)));
}

inline int SplitX(double progress, int width) {
  if (width <= 0) {
    return 0;
  }
  const double clamped = std::clamp(progress, 0.0, 1.0);
  return static_cast<int>(clamped * static_cast<double>(width) + 0.0);
}

inline bool ShowPlayhead(double progress) { return progress >= 0.0; }

inline bool ShowPlayheadFromPosition(int64_t position_nanosec) { return position_nanosec >= 0; }

}  // namespace WaveformPlayhead

#endif
