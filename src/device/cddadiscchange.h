#ifndef STRAWBERRY_CDDADISCCHANGE_H
#define STRAWBERRY_CDDADISCCHANGE_H

namespace CddaDiscChange {

// Qt CDDADevice::timer_disc_changed_ interval is 1s.
inline constexpr int kPollMs = 1000;

inline bool ShouldCheck(bool has_handle, bool loader_active) { return has_handle && !loader_active; }

// cdio_get_media_changed returns 1 when the disc changed.
inline bool MediaChanged(int cdio_result) { return cdio_result == 1; }

inline bool ShouldStartWatch(bool watch, bool already_active) { return watch && !already_active; }

inline bool ShouldStopWatch(bool watch, bool already_active) { return !watch && already_active; }

inline bool ShouldPauseWatchWhileLoading() { return true; }

inline bool ShouldAckAfterLoad() { return true; }

}  // namespace CddaDiscChange

#endif
