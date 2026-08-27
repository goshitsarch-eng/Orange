#ifndef STRAWBERRY_SCROBBLERLIFECYCLE_H
#define STRAWBERRY_SCROBBLERLIFECYCLE_H

namespace ScrobblerLifecycle {

inline bool ShouldFlushOnExit(bool enabled) { return enabled; }

inline bool ShouldSubmitAfterOfflineToggle(bool was_offline, bool now_offline) { return was_offline && !now_offline; }

// Qt MainWindow constructor: Submit cached scrobbles when enabled and online.
inline bool ShouldSubmitOnStartup(bool enabled, bool offline) { return enabled && !offline; }

}  // namespace ScrobblerLifecycle

#endif  // STRAWBERRY_SCROBBLERLIFECYCLE_H
