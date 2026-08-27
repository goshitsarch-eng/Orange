#ifndef STRAWBERRY_SMARTPLAYLISTDATEUNITS_H
#define STRAWBERRY_SMARTPLAYLISTDATEUNITS_H

#include "smartplaylists/smartplaylist.h"

#include <glib.h>

#include <string>
#include <vector>

namespace SmartPlaylistDateUnits {

// Qt SmartPlaylistSearchTerm::DateName
inline const char *Name(SmartPlaylistDateType type) {
  switch (type) {
    case SmartPlaylistDateType::Hour:
      return "Hours";
    case SmartPlaylistDateType::Week:
      return "Weeks";
    case SmartPlaylistDateType::Month:
      return "Months";
    case SmartPlaylistDateType::Year:
      return "Years";
    case SmartPlaylistDateType::Day:
    default:
      return "Days";
  }
}

inline const char *QueryName(SmartPlaylistDateType type) {
  switch (type) {
    case SmartPlaylistDateType::Hour:
      return "hours";
    case SmartPlaylistDateType::Week:
      return "weeks";
    case SmartPlaylistDateType::Month:
      return "months";
    case SmartPlaylistDateType::Year:
      return "years";
    case SmartPlaylistDateType::Day:
    default:
      return "days";
  }
}

inline std::vector<std::string> Names() { return {"Hours", "Days", "Weeks", "Months", "Years"}; }

inline SmartPlaylistDateType FromIndex(int index) {
  if (index < 0 || index > static_cast<int>(SmartPlaylistDateType::Year)) {
    return SmartPlaylistDateType::Day;
  }
  return static_cast<SmartPlaylistDateType>(index);
}

inline int ToIndex(SmartPlaylistDateType type) { return static_cast<int>(type); }

inline int64_t SecondsFor(SmartPlaylistDateType type, int64_t count) {
  if (count < 0) {
    count = 0;
  }
  switch (type) {
    case SmartPlaylistDateType::Hour:
      return count * 3600;
    case SmartPlaylistDateType::Week:
      return count * 7 * 86400;
    case SmartPlaylistDateType::Month:
      return count * 30 * 86400;
    case SmartPlaylistDateType::Year:
      return count * 365 * 86400;
    case SmartPlaylistDateType::Day:
    default:
      return count * 86400;
  }
}

inline int64_t CutoffUnix(SmartPlaylistDateType type, int64_t count) {
  if (count < 0) {
    count = 0;
  }
  const gint amount = static_cast<gint>(count > 200000 ? 200000 : count);
  GDateTime *now = g_date_time_new_now_utc();
  GDateTime *then = nullptr;
  switch (type) {
    case SmartPlaylistDateType::Hour:
      then = g_date_time_add_hours(now, -amount);
      break;
    case SmartPlaylistDateType::Week:
      then = g_date_time_add_weeks(now, -amount);
      break;
    case SmartPlaylistDateType::Month:
      then = g_date_time_add_months(now, -amount);
      break;
    case SmartPlaylistDateType::Year:
      then = g_date_time_add_years(now, -amount);
      break;
    case SmartPlaylistDateType::Day:
    default:
      then = g_date_time_add_days(now, -amount);
      break;
  }
  const int64_t unix_time = then ? g_date_time_to_unix(then) : 0;
  if (then) {
    g_date_time_unref(then);
  }
  g_date_time_unref(now);
  return unix_time;
}

// Qt NumericDate: col > DATETIME('now', -N, unit)
inline bool InTheLast(int64_t have, SmartPlaylistDateType type, int64_t count) {
  return have > 0 && have > CutoffUnix(type, count);
}

// Qt NumericDateNot: col < DATETIME('now', -N, unit)
inline bool NotInTheLast(int64_t have, SmartPlaylistDateType type, int64_t count) {
  return have < CutoffUnix(type, count);
}

// Qt RelativeDate: nearer first, farther second.
inline bool Between(int64_t have, SmartPlaylistDateType type, int64_t first, int64_t second) {
  if (first >= second) {
    return false;
  }
  return have < CutoffUnix(type, first) && have > CutoffUnix(type, second);
}

}  // namespace SmartPlaylistDateUnits

#endif
