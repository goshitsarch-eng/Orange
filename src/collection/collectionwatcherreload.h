#ifndef STRAWBERRY_COLLECTIONWATCHERRELOAD_H
#define STRAWBERRY_COLLECTIONWATCHERRELOAD_H

namespace CollectionWatcherReload {

// Qt CollectionLibrary::ReloadSettings / CollectionWatcher::ReloadSettings after Preferences.

inline bool ShouldReloadOnSettingsClose() { return true; }

inline bool MarkUnavailable(bool song_tracking, bool mark_unavailable) { return song_tracking ? true : mark_unavailable; }

inline bool ShouldRunPeriodicScan(bool monitor, bool startup_scan, bool mark_unavailable) {
  return monitor && startup_scan && mark_unavailable;
}

inline bool ShouldStopWatching(bool was_monitoring, bool monitor_now) { return was_monitoring && !monitor_now; }

inline bool ShouldStartWatching(bool was_monitoring, bool monitor_now) { return !was_monitoring && monitor_now; }

}  // namespace CollectionWatcherReload

#endif
