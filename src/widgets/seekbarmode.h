#ifndef STRAWBERRY_SEEKBARMODE_H
#define STRAWBERRY_SEEKBARMODE_H

#include "core/seekbarsettings.h"

namespace SeekbarModeMenu {

constexpr int kCount = 3;

inline SeekbarSettings::Mode Clamp(int mode) {
  if (mode < 0 || mode >= kCount) {
    return SeekbarSettings::kDefaultMode;
  }
  return static_cast<SeekbarSettings::Mode>(mode);
}

inline SeekbarSettings::Mode Next(SeekbarSettings::Mode mode) {
  return static_cast<SeekbarSettings::Mode>((static_cast<int>(mode) + 1) % kCount);
}

inline const char *Label(SeekbarSettings::Mode mode) {
  switch (mode) {
    case SeekbarSettings::Mode::Moodbar:
      return "Moodbar";
    case SeekbarSettings::Mode::Waveform:
      return "Waveform";
    case SeekbarSettings::Mode::Normal:
    default:
      return "Normal";
  }
}

}  // namespace SeekbarModeMenu

#endif
