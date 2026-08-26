#ifndef STRAWBERRY_SCROBBLERELIGIBILITY_H
#define STRAWBERRY_SCROBBLERELIGIBILITY_H

#include "core/song.h"

#include <algorithm>
#include <cstdint>

namespace ScrobblerEligibility {

constexpr int64_t kMinLengthNanosec = 30LL * 1000000000LL;
constexpr int64_t kFourMinuteNanosec = 240LL * 1000000000LL;

inline bool ShouldScrobble(const Song &song, int64_t listened_nanosec, int submit_percent = 0) {
  if (!song.is_valid() && song.url().empty()) {
    return false;
  }
  if (song.title().empty() && song.artist().empty()) {
    return false;
  }
  const int64_t listened = std::max<int64_t>(0, listened_nanosec);
  const int64_t length = song.length_nanosec();
  if (length > 0 && length < kMinLengthNanosec) {
    return false;
  }
  if (submit_percent > 0 && submit_percent <= 100 && length > 0) {
    return listened >= length * submit_percent / 100;
  }
  if (length > 0) {
    return listened >= std::min(kFourMinuteNanosec, length / 2);
  }
  return listened >= kFourMinuteNanosec;
}

}  // namespace ScrobblerEligibility

#endif
