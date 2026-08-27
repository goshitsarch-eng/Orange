#ifndef STRAWBERRY_ENGINEABOUTTOEND_H
#define STRAWBERRY_ENGINEABOUTTOEND_H

#include "engine/backendoptions.h"

#include <algorithm>
#include <cstdint>

namespace EngineAboutToEnd {

// Qt GstEngine timerEvent: poll every 1 s and fire when remaining < buffer + (autocrossfade ? fade : 8 s) + fudge.
inline constexpr int kTimerIntervalMs = 1000;
inline constexpr int64_t kPreloadGapNanosec = 8000 * BackendOptions::kNsecPerMsec;
inline constexpr int64_t kFudgeNanosec = kTimerIntervalMs * BackendOptions::kNsecPerMsec + 100 * BackendOptions::kNsecPerMsec;

inline int64_t FadeDurationNanosec(int fade_ms) {
  return static_cast<int64_t>(std::max(100, fade_ms)) * BackendOptions::kNsecPerMsec;
}

inline int64_t LeadTimeNanosec(int64_t buffer_ms, bool autocrossfade, int fade_ms) {
  return BackendOptions::BufferDurationNanosec(buffer_ms) + (autocrossfade ? FadeDurationNanosec(fade_ms) : kPreloadGapNanosec);
}

inline bool ShouldEmit(int64_t remaining_ns, int64_t length_ns, int64_t lead_ns, int64_t fudge_ns, bool already_emitted) {
  return !already_emitted && length_ns > 0 && remaining_ns < lead_ns + fudge_ns;
}

}  // namespace EngineAboutToEnd

#endif
