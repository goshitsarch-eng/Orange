#ifndef STRAWBERRY_TRACKSLIDERTIME_H
#define STRAWBERRY_TRACKSLIDERTIME_H

#include "core/seekbarsettings.h"
#include "utilities/timeutils.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace TrackSliderTime {

inline const char *SettingsGroup() { return SeekbarSettings::kSettingsGroup; }
inline const char *SettingsKey() { return SeekbarSettings::kShowRemaining; }
inline bool DefaultShowRemaining() { return false; }

inline std::string PositionLabel(int64_t position_nanosec) { return Utilities::PrettyTimeNanosec(position_nanosec); }

inline std::string DurationLabel(bool show_remaining, int64_t position_nanosec, int64_t length_nanosec) {
  if (!show_remaining) {
    return Utilities::PrettyTimeNanosec(length_nanosec);
  }
  const int64_t remaining = std::max<int64_t>(0, length_nanosec - position_nanosec);
  return "-" + Utilities::PrettyTimeNanosec(remaining);
}

inline std::string PopupText(bool show_remaining, int64_t position_nanosec, int64_t length_nanosec) {
  return PositionLabel(position_nanosec) + " / " + DurationLabel(show_remaining, position_nanosec, length_nanosec);
}

inline const char *DurationTooltip() { return "Click to toggle between remaining time and total time"; }

}  // namespace TrackSliderTime

#endif
