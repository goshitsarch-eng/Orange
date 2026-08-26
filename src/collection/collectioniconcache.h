#ifndef STRAWBERRY_COLLECTIONICONCACHE_H
#define STRAWBERRY_COLLECTIONICONCACHE_H

#include "constants/collectionsettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"
#include "utilities/fileutils.h"

#include <glib.h>

#include <cstdint>
#include <string>

namespace CollectionIconCache {

inline constexpr const char *kPixmapDiskCacheDir = "pixmapcache";
inline constexpr const char *kSourceName = "Collection";

inline std::string CacheDir() {
  const std::string path = FileUtils::Join(StandardPaths::CacheDir(), std::string(kPixmapDiskCacheDir) + "-" + kSourceName);
  g_mkdir_with_parents(path.c_str(), 0755);
  return path;
}

inline std::string SafeKey(const std::string &key) {
  std::string out;
  out.reserve(key.size() * 3);
  for (unsigned char c : key) {
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-' || c == '_' || c == '.') {
      out += static_cast<char>(c);
    } else {
      char buf[8] = {};
      g_snprintf(buf, sizeof(buf), "%%%02X", c);
      out += buf;
    }
  }
  return out.empty() ? "cover" : out;
}

inline std::string CacheFile(const std::string &key, const std::string &dir = {}) {
  return FileUtils::Join(dir.empty() ? CacheDir() : dir, SafeKey(key) + ".img");
}

inline int64_t MaximumCacheSize(int size, int unit) {
  int64_t bytes = size;
  int steps = unit + 1;
  while (steps > 0) {
    bytes *= 1024;
    --steps;
  }
  return bytes;
}

inline int64_t MaximumCacheSizeFromSettings() {
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  return MaximumCacheSize(settings.IntValue(CollectionSettings::kSettingsDiskCacheSize, CollectionSettings::kSettingsDiskCacheSizeDefault),
                          settings.IntValue(CollectionSettings::kSettingsDiskCacheSizeUnit,
                                            static_cast<int>(CollectionSettings::kDefaultSettingsDiskCacheSizeUnit)));
}

inline bool DiskCacheEnabled() {
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  return settings.BoolValue(CollectionSettings::kSettingsDiskCacheEnable, CollectionSettings::kDefaultSettingsDiskCacheEnable);
}

inline int64_t DiskCacheBytes(const std::string &dir) {
  if (!FileUtils::IsDirectory(dir)) {
    return 0;
  }
  int64_t total = 0;
  for (const std::string &path : FileUtils::ListDirectoryRecursive(dir)) {
    const int64_t size = FileUtils::FileSize(path);
    if (size > 0) {
      total += size;
    }
  }
  return total;
}

inline int64_t DiskCacheBytes() { return DiskCacheBytes(CacheDir()); }

inline std::string InUseLabel(int64_t bytes) { return bytes == 0 ? "empty" : FileUtils::PrettySize(bytes); }

inline std::string InUseLabel() { return InUseLabel(DiskCacheBytes()); }

inline void Clear(const std::string &dir) {
  if (!FileUtils::IsDirectory(dir)) {
    return;
  }
  for (const std::string &path : FileUtils::ListDirectoryRecursive(dir)) {
    FileUtils::Remove(path);
  }
}

inline void Clear() { Clear(CacheDir()); }

inline std::string Read(const std::string &key, const std::string &dir = {}) {
  const std::string path = CacheFile(key, dir);
  if (!FileUtils::Exists(path)) {
    return {};
  }
  return FileUtils::ReadFile(path);
}

inline bool Write(const std::string &key, const std::string &data, const std::string &dir, int64_t max_bytes) {
  if (key.empty() || data.empty()) {
    return false;
  }
  if (DiskCacheBytes(dir) + static_cast<int64_t>(data.size()) > max_bytes) {
    return false;
  }
  g_mkdir_with_parents(dir.c_str(), 0755);
  return FileUtils::WriteFile(CacheFile(key, dir), data);
}

inline bool Write(const std::string &key, const std::string &data) {
  if (!DiskCacheEnabled()) {
    return false;
  }
  return Write(key, data, CacheDir(), MaximumCacheSizeFromSettings());
}

}  // namespace CollectionIconCache

#endif
