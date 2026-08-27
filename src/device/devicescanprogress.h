#ifndef STRAWBERRY_DEVICESCANPROGRESS_H
#define STRAWBERRY_DEVICESCANPROGRESS_H

#include <algorithm>
#include <string>

namespace DeviceScanProgress {

inline const char *TaskName() { return "Scanning device"; }

inline int Percent(int done, int total) {
  if (total <= 0) {
    return 0;
  }
  return std::clamp(done * 100 / total, 0, 100);
}

inline bool ShouldReport(int previous_percent, int percent) { return percent != previous_percent; }

inline bool IsScanTask(const std::string &name) { return name == TaskName(); }

inline int FinishedPercent() { return -1; }

}  // namespace DeviceScanProgress

#endif  // STRAWBERRY_DEVICESCANPROGRESS_H
