#ifndef DISCORDRECONNECT_H
#define DISCORDRECONNECT_H

#include <glib.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace DiscordReconnect {

// Qt DiscordRPC reconnect_timer_ backoff.
inline constexpr int kMinDelayMs = 500;
inline constexpr int kMaxDelayMs = 60000;
inline constexpr int kSocketCount = 10;

inline int NextDelay(int current_ms) {
  if (current_ms < kMinDelayMs) {
    current_ms = kMinDelayMs;
  }
  return std::min(current_ms * 2, kMaxDelayMs);
}

inline bool ShouldSchedule(bool enabled, bool shutting_down, bool already_pending) {
  return enabled && !shutting_down && !already_pending;
}

inline std::vector<std::string> TempDirs() {
  std::vector<std::string> dirs;
  std::set<std::string> seen;
  const auto add = [&](const char *value) {
    if (!value || !*value) {
      return;
    }
    if (seen.insert(value).second) {
      dirs.emplace_back(value);
    }
  };
  add(g_getenv("XDG_RUNTIME_DIR"));
  add(g_getenv("TMPDIR"));
  add(g_getenv("TMP"));
  add(g_getenv("TEMP"));
  add("/tmp");
  return dirs;
}

inline std::string SocketPath(const std::string &dir, int index) { return dir + "/discord-ipc-" + std::to_string(index); }

inline std::vector<std::string> SocketPaths() {
  std::vector<std::string> paths;
  for (const std::string &dir : TempDirs()) {
    for (int i = 0; i < kSocketCount; ++i) {
      paths.push_back(SocketPath(dir, i));
    }
  }
  return paths;
}

}  // namespace DiscordReconnect

#endif  // DISCORDRECONNECT_H
