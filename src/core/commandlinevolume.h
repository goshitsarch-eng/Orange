#ifndef STRAWBERRY_COMMANDLINEVOLUME_H
#define STRAWBERRY_COMMANDLINEVOLUME_H

#include <algorithm>

namespace CommandlineVolume {

// Qt commandlineoptions.cpp LongOptions::VolumeUp / VolumeDown hard-code ±4.
inline constexpr int kUpDownStep = 4;

inline int Modifier(bool volume_up, bool volume_down, int increase_by, int decrease_by) {
  if (volume_up) {
    return kUpDownStep;
  }
  if (volume_down) {
    return -kUpDownStep;
  }
  return increase_by - decrease_by;
}

inline int Apply(int current, int modifier) { return std::clamp(current + modifier, 0, 100); }

}  // namespace CommandlineVolume

#endif
