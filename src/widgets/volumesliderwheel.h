#ifndef STRAWBERRY_VOLUMESLIDERWHEEL_H
#define STRAWBERRY_VOLUMESLIDERWHEEL_H

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace VolumeSliderWheel {

// Qt VolumeSlider::WHEEL_ROTATION_PER_STEP, in eighths of a degree.
constexpr int kRotationPerStep = 30;
constexpr unsigned kMax = 100;

struct Result {
  int steps = 0;
  int accumulator = 0;
};

inline Result FromAngleDelta(int accumulator, int angle_delta_y) {
  const int state = accumulator + angle_delta_y;
  Result result;
  result.steps = state / kRotationPerStep;
  result.accumulator = state % kRotationPerStep;
  return result;
}

// GTK scroll dy is positive downward. Qt angleDelta.y is positive upward and raises volume.
inline Result FromGtkScroll(int accumulator, double dy) {
  return FromAngleDelta(accumulator, static_cast<int>(std::lround(-dy * static_cast<double>(kRotationPerStep))));
}

inline unsigned ApplySteps(unsigned volume, int steps, unsigned max = kMax) {
  const int next = static_cast<int>(volume) + steps;
  return static_cast<unsigned>(std::clamp(next, 0, static_cast<int>(max)));
}

inline std::string PercentLabel(unsigned volume) { return std::to_string(volume) + "%"; }

inline std::array<int, 6> Presets() { return {100, 80, 60, 40, 20, 0}; }

}  // namespace VolumeSliderWheel

#endif
