#ifndef STRAWBERRY_SKIPCOUNTELIGIBILITY_H
#define STRAWBERRY_SKIPCOUNTELIGIBILITY_H

#include <algorithm>
#include <cstdint>

namespace SkipCountEligibility {

constexpr int64_t kNsecPerSec = 1000000000LL;

inline bool ShouldIncrement(int64_t pos_ns, int64_t len_ns) {
  if (len_ns <= 0) {
    return false;
  }
  const double seconds_total = static_cast<double>(len_ns) / static_cast<double>(kNsecPerSec);
  const double seconds_pos = static_cast<double>(std::max<int64_t>(0, pos_ns)) / static_cast<double>(kNsecPerSec);
  const double seconds_left = seconds_total - seconds_pos;
  const double percentage = seconds_pos / seconds_total;
  return ((0.05 * seconds_total > 60.0 && percentage < 0.98) || percentage < 0.95) && seconds_left > 5.0;
}

}  // namespace SkipCountEligibility

#endif  // STRAWBERRY_SKIPCOUNTELIGIBILITY_H
