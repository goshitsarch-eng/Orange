#ifndef STRAWBERRY_ENGINEBUFFERING_H
#define STRAWBERRY_ENGINEBUFFERING_H

#include <cstdint>

namespace EngineBuffering {

constexpr int kIgnoreNearEndSeconds = 5;
constexpr int64_t kNsecPerSec = 1000000000LL;
constexpr int kProgressMax = 100;

inline bool IgnoreNearEnd(bool about_to_finish, int64_t position_nanosec, int64_t length_nanosec) {
  return about_to_finish && length_nanosec > 0 && position_nanosec > 0 &&
         (length_nanosec - position_nanosec) < static_cast<int64_t>(kIgnoreNearEndSeconds) * kNsecPerSec;
}

inline bool ShouldStart(int percent, bool already_buffering) { return percent < kProgressMax && !already_buffering; }

inline bool ShouldFinish(int percent, bool already_buffering) { return percent >= kProgressMax && already_buffering; }

inline const char *TaskName() { return "Buffering"; }

}  // namespace EngineBuffering

#endif  // STRAWBERRY_ENGINEBUFFERING_H
