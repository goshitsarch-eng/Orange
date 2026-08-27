#ifndef STRAWBERRY_SMARTPLAYLISTTERMVALUE_H
#define STRAWBERRY_SMARTPLAYLISTTERMVALUE_H

#include "smartplaylists/smartplaylist.h"

#include <cstdio>
#include <string>

namespace SmartPlaylistTermValue {

enum class Editor { Empty, RelativeDays, Calendar, Rating, Time, Number, Text };

// Qt SmartPlaylistSearchTermWidget::FieldChanged picks the value stack page from field type and operator.
inline Editor EditorFor(SmartPlaylistFieldKind kind, SmartPlaylistOp op) {
  if (op == SmartPlaylistOp::Empty || op == SmartPlaylistOp::NotEmpty) {
    return Editor::Empty;
  }
  if (op == SmartPlaylistOp::RelativeDate) {
    return Editor::RelativeDays;
  }
  if (kind == SmartPlaylistFieldKind::Date || op == SmartPlaylistOp::NumericDate) {
    return Editor::Calendar;
  }
  if (kind == SmartPlaylistFieldKind::Rating) {
    return Editor::Rating;
  }
  if (kind == SmartPlaylistFieldKind::Time) {
    return Editor::Time;
  }
  if (kind == SmartPlaylistFieldKind::Number) {
    return Editor::Number;
  }
  return Editor::Text;
}

inline std::string FormatDate(int year, int month, int day) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
  return buf;
}

inline bool ParseDate(const std::string &value, int *year, int *month, int *day) {
  int y = 0;
  int m = 0;
  int d = 0;
  if (std::sscanf(value.c_str(), "%d-%d-%d", &y, &m, &d) != 3 || y <= 1900 || m < 1 || m > 12 || d < 1 || d > 31) {
    return false;
  }
  if (year) {
    *year = y;
  }
  if (month) {
    *month = m;
  }
  if (day) {
    *day = d;
  }
  return true;
}

inline int TimeToSeconds(int hours, int minutes, int seconds) {
  if (hours < 0) {
    hours = 0;
  }
  if (minutes < 0) {
    minutes = 0;
  }
  if (seconds < 0) {
    seconds = 0;
  }
  if (seconds > 59) {
    seconds = 59;
  }
  return hours * 3600 + minutes * 60 + seconds;
}

inline void SecondsToTime(int total, int *hours, int *minutes, int *seconds) {
  if (total < 0) {
    total = 0;
  }
  if (hours) {
    *hours = total / 3600;
  }
  if (minutes) {
    *minutes = (total % 3600) / 60;
  }
  if (seconds) {
    *seconds = total % 60;
  }
}

// Qt RatingWidget stores a float; Matches() parses it with strtod.
inline std::string FormatRating(float rating) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(rating));
  return buf;
}

}  // namespace SmartPlaylistTermValue

#endif
