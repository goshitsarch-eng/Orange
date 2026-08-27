#include "utilities/timeutils.h"

#include <cstdio>
#include <sstream>

namespace Utilities {

std::string PrettyTime(int seconds) {
  if (seconds < 0) {
    seconds = 0;
  }
  const int hours = seconds / 3600;
  const int minutes = (seconds % 3600) / 60;
  const int secs = seconds % 60;
  char buffer[32];
  if (hours > 0) {
    std::snprintf(buffer, sizeof(buffer), "%d:%02d:%02d", hours, minutes, secs);
  } else {
    std::snprintf(buffer, sizeof(buffer), "%d:%02d", minutes, secs);
  }
  return buffer;
}

std::string PrettyTimeDelta(int seconds) {
  const bool negative = seconds < 0;
  std::string result = PrettyTime(negative ? -seconds : seconds);
  return negative ? "-" + result : "+" + result;
}

std::string PrettyTimeNanosec(int64_t nanoseconds) {
  return PrettyTime(static_cast<int>(nanoseconds / 1000000000LL));
}

std::string WordyTime(uint64_t seconds) {
  const uint64_t hours = seconds / 3600;
  const uint64_t minutes = (seconds % 3600) / 60;
  std::ostringstream out;
  if (hours > 0) {
    out << hours << (hours == 1 ? " hour" : " hours");
    if (minutes > 0) {
      out << " " << minutes << (minutes == 1 ? " minute" : " minutes");
    }
  } else {
    out << minutes << (minutes == 1 ? " minute" : " minutes");
  }
  return out.str();
}

std::string WordyTimeNanosec(uint64_t nanoseconds) { return WordyTime(nanoseconds / 1000000000ULL); }

}  // namespace Utilities
