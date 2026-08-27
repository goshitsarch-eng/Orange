#ifndef STRAWBERRY_COVEREXPORTJOB_H
#define STRAWBERRY_COVEREXPORTJOB_H

#include <algorithm>
#include <string>

namespace CoverExportJob {

inline constexpr int kMaxConcurrent = 3;

inline bool ShouldProcessBatch(bool cancelled) { return !cancelled; }

inline bool ShouldFinish(int next, int total, bool cancelled) { return cancelled || next >= total; }

inline bool ShouldScheduleNext(int next, int total, bool cancelled, bool async) {
  return async && !cancelled && next < total;
}

inline bool ShouldPump(int active, int pending) { return active < kMaxConcurrent && pending > 0; }

inline double ProgressFraction(int exported, int skipped, int total) {
  if (total <= 0) {
    return 0.0;
  }
  return std::clamp(static_cast<double>(exported + skipped) / static_cast<double>(total), 0.0, 1.0);
}

inline std::string StatusText(int exported, int skipped, int total) {
  return "Exported " + std::to_string(exported) + " of " + std::to_string(total) + " (" + std::to_string(skipped) + " skipped)";
}

}  // namespace CoverExportJob

#endif  // STRAWBERRY_COVEREXPORTJOB_H
