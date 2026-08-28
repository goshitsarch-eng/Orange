#ifndef STRAWBERRY_SPLITTERSTATE_H
#define STRAWBERRY_SPLITTERSTATE_H

#include <algorithm>

namespace SplitterState {

inline constexpr double kDefaultFraction = 0.30;
inline constexpr double kMinFraction = 0.12;
inline constexpr double kMaxFraction = 0.70;

inline double Clamp(double fraction) {
  if (fraction <= 0.0) {
    return kDefaultFraction;
  }
  return std::clamp(fraction, kMinFraction, kMaxFraction);
}

inline double Restore(bool contains, double saved) { return contains ? Clamp(saved) : kDefaultFraction; }

}  // namespace SplitterState

#endif
