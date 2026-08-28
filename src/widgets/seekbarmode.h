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

inline bool IsChecked(SeekbarSettings::Mode mode, SeekbarSettings::Mode current) { return mode == current; }

inline bool StyleMenuEnabled(SeekbarSettings::Mode mode) { return mode == SeekbarSettings::Mode::Moodbar; }

inline const char *StyleSubmenuTitle() { return "Moodbar style"; }

// Qt TrackSlider / moodbar / waveform context menus also open from Menu / Shift+F10.
constexpr unsigned kMenu = 0xff67;
constexpr unsigned kF10 = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;

inline bool IsKeyboardTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenu || (keyval == kF10 && (state & kShiftMask) != 0);
}

}  // namespace SeekbarModeMenu

#endif
