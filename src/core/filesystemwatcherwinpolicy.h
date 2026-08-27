#ifndef STRAWBERRY_FILESYSTEMWATCHERWINPOLICY_H
#define STRAWBERRY_FILESYSTEMWATCHERWINPOLICY_H

#include "utilities/strutils.h"

#include <algorithm>
#include <string>

namespace FileSystemWatcherWinPolicy {

// WaitForMultipleObjects can wait on at most 64 handles; slot 0 is the wakeup event.
inline constexpr int kMaxWaitObjects = 64;
inline constexpr int kWakeupSlots = 1;
// FILE_NOTIFY_CHANGE_FILE_NAME|DIR_NAME|ATTRIBUTES|SIZE|LAST_WRITE — same mask as Qt.
inline constexpr unsigned kNotifyFlags = 0x0000001F;
inline constexpr bool kWatchSubtree = false;

inline int MaxWatchesPerThread() { return kMaxWaitObjects - kWakeupSlots; }

inline bool ThreadHasRoom(int watch_count) { return watch_count < MaxWatchesPerThread(); }

inline std::string PathKey(const std::string &path) {
  std::string key = path;
  std::replace(key.begin(), key.end(), '/', '\\');
  return StrUtils::ToLower(key);
}

}  // namespace FileSystemWatcherWinPolicy

#endif
