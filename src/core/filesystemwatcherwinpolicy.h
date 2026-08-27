#ifndef STRAWBERRY_FILESYSTEMWATCHERWINPOLICY_H
#define STRAWBERRY_FILESYSTEMWATCHERWINPOLICY_H

#include "utilities/strutils.h"

#include <algorithm>
#include <string>

namespace FileSystemWatcherWinPolicy {

// WaitForMultipleObjects can wait on at most 64 handles; slot 0 is the wakeup event.
inline constexpr int kMaxWaitObjects = 64;
inline constexpr int kWakeupSlots = 1;

inline int MaxWatchesPerThread() { return kMaxWaitObjects - kWakeupSlots; }

inline bool ThreadHasRoom(int watch_count) { return watch_count < MaxWatchesPerThread(); }

inline std::string PathKey(const std::string &path) {
  std::string key = path;
  std::replace(key.begin(), key.end(), '/', '\\');
  return StrUtils::ToLower(key);
}

}  // namespace FileSystemWatcherWinPolicy

#endif
