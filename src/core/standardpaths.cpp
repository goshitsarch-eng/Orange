#include "core/standardpaths.h"

#include <glib.h>
#include <sys/stat.h>

#include <string>

namespace {

void EnsureDir(const std::string &path) { g_mkdir_with_parents(path.c_str(), 0755); }

std::string Join(const std::string &a, const std::string &b) {
  if (a.empty()) {
    return b;
  }
  if (a.back() == '/') {
    return a + b;
  }
  return a + "/" + b;
}

}  // namespace

namespace StandardPaths {

std::string ConfigDir() {
  const char *dir = g_get_user_config_dir();
  const std::string path = Join(dir ? dir : "", "strawberry");
  EnsureDir(path);
  return path;
}

std::string DataDir() {
  const char *dir = g_get_user_data_dir();
  const std::string path = Join(dir ? dir : "", "strawberry");
  EnsureDir(path);
  return path;
}

std::string CacheDir() {
  const char *dir = g_get_user_cache_dir();
  const std::string path = Join(dir ? dir : "", "strawberry");
  EnsureDir(path);
  return path;
}

std::string DatabasePath() { return Join(DataDir(), "strawberry.db"); }

std::string SettingsPath() { return Join(ConfigDir(), "strawberry.conf"); }

std::string CoverCacheDir() {
  const std::string path = Join(CacheDir(), "albumcovers");
  EnsureDir(path);
  return path;
}

std::string LyricsCacheDir() {
  const std::string path = Join(CacheDir(), "lyrics");
  EnsureDir(path);
  return path;
}

std::string MoodbarCacheDir() {
  const std::string path = Join(CacheDir(), "moodbar");
  EnsureDir(path);
  return path;
}

std::string WaveformCacheDir() {
  const std::string path = Join(CacheDir(), "waveform");
  EnsureDir(path);
  return path;
}

std::string LocaleDir() {
  if (const char *env = g_getenv("STRAWBERRY_LOCALE_DIR")) {
    return env;
  }
#ifdef STRAWBERRY_LOCALE_DIR
  return STRAWBERRY_LOCALE_DIR;
#else
  return "/usr/share/locale";
#endif
}

}  // namespace StandardPaths
