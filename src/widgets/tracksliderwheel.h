#ifndef STRAWBERRY_TRACKSLIDERWHEEL_H
#define STRAWBERRY_TRACKSLIDERWHEEL_H

#include "utilities/timeutils.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace TrackSliderWheel {

// Qt TrackSliderSlider::WHEEL_ROTATION_TO_SEEK, in eighths of a degree.
constexpr int kRotationToSeek = 120;

struct Result {
  int steps = 0;
  int accumulator = 0;
};

inline Result FromAngleDelta(int accumulator, int angle_delta_y) {
  const int state = accumulator + angle_delta_y;
  Result result;
  result.steps = state / kRotationToSeek;
  result.accumulator = state % kRotationToSeek;
  return result;
}

// GTK scroll dy is positive downward. Qt angleDelta.y is positive upward and seeks forward.
inline Result FromGtkScroll(int accumulator, double dy) {
  return FromAngleDelta(accumulator, static_cast<int>(std::lround(-dy * static_cast<double>(kRotationToSeek))));
}

enum class Direction { None, Backward, Forward };

inline Direction DirectionFromSteps(int steps) {
  if (steps < 0) {
    return Direction::Backward;
  }
  if (steps > 0) {
    return Direction::Forward;
  }
  return Direction::None;
}

}  // namespace TrackSliderWheel

namespace TrackSliderHover {

inline int SecondsAtX(double x, double width, int length_sec) {
  if (width <= 0.0 || length_sec <= 0) {
    return 0;
  }
  const double t = std::clamp(x / width, 0.0, 1.0);
  return static_cast<int>(std::lround(t * static_cast<double>(length_sec)));
}

inline std::string HoverText(int hover_sec) { return Utilities::PrettyTime(hover_sec); }

inline std::string DeltaText(int hover_sec, int current_sec) { return Utilities::PrettyTimeDelta(hover_sec - current_sec); }

}  // namespace TrackSliderHover

#endif
