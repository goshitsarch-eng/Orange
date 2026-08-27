#ifndef STRAWBERRY_MOODBARPLAYHEAD_H
#define STRAWBERRY_MOODBARPLAYHEAD_H

#include "waveform/waveformplayhead.h"

#include <algorithm>
#include <cstdint>

namespace MoodbarPlayhead {

// Qt MoodbarProxyStyle seekbar chrome.
inline constexpr int kMarginSize = 3;
inline constexpr int kBorderSize = 1;
inline constexpr int kArrowWidth = 17;
inline constexpr int kArrowHeight = 13;

inline int InnerWidth(int width) { return std::max(0, width - 2 * (kMarginSize + kBorderSize)); }

inline int InnerHeight(int height) { return std::max(0, height - 2 * (kMarginSize + kBorderSize)); }

inline int ArrowTravel(int width) { return std::max(0, width - kArrowWidth); }

inline int ArrowLeft(int64_t position_nanosec, int64_t length_nanosec, int width) {
  const int travel = ArrowTravel(width);
  if (travel <= 0 || length_nanosec <= 0) {
    return 0;
  }
  const int64_t x = position_nanosec * static_cast<int64_t>(travel) / length_nanosec;
  return static_cast<int>(std::clamp(x, static_cast<int64_t>(0), static_cast<int64_t>(travel)));
}

inline int ArrowLeft(double progress, int width) {
  const int travel = ArrowTravel(width);
  if (travel <= 0) {
    return 0;
  }
  const double clamped = std::clamp(progress, 0.0, 1.0);
  return static_cast<int>(clamped * static_cast<double>(travel));
}

inline int ArrowCenterX(int left) { return left + kArrowWidth / 2; }

inline bool ShowPlayhead(double progress) { return WaveformPlayhead::ShowPlayhead(progress); }

inline bool ShowPlayheadFromPosition(int64_t position_nanosec) { return WaveformPlayhead::ShowPlayheadFromPosition(position_nanosec); }

}  // namespace MoodbarPlayhead

#endif
