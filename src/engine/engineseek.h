#ifndef STRAWBERRY_ENGINESEEK_H
#define STRAWBERRY_ENGINESEEK_H

namespace EngineSeek {

// Qt GstEngine: kSeekDelayNanosec = 100ms.
constexpr int kDelayMs = 100;

// First seek is immediate; later seeks while the delay timer is active are coalesced.
inline bool ShouldSeekImmediately(bool timer_active) { return !timer_active; }

inline bool ShouldApplyPending(bool waiting_to_seek) { return waiting_to_seek; }

}  // namespace EngineSeek

#endif  // STRAWBERRY_ENGINESEEK_H
