#ifndef STRAWBERRY_COLLECTIONDIRECTORYART_H
#define STRAWBERRY_COLLECTIONDIRECTORYART_H

#include "constants/collectionsettings.h"
#include "core/settings.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <string>
#include <vector>

namespace CollectionDirectoryArt {

inline std::vector<std::string> DefaultFilters() { return {"front", "cover"}; }

inline bool IsImageFile(const std::string &path) {
  const std::string ext = StrUtils::ToLower(FileUtils::Extension(path));
  return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "webp" || ext == "bmp";
}

inline std::vector<std::string> ParseFilters(const std::string &raw) {
  std::vector<std::string> filters;
  for (std::string part : StrUtils::Split(raw, ',')) {
    part = StrUtils::Trim(part);
    if (!part.empty()) {
      filters.push_back(part);
    }
  }
  return filters.empty() ? DefaultFilters() : filters;
}

inline std::vector<std::string> FiltersFromSettings() {
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  return ParseFilters(settings.Value(CollectionSettings::kCoverArtPatterns, "front,cover"));
}

inline bool NameMatches(const std::string &path, const std::string &filter) {
  return StrUtils::ContainsInsensitive(FileUtils::BaseName(path), filter);
}

inline std::string PickBestArt(const std::vector<std::string> &paths, const std::vector<std::string> &filters) {
  std::vector<std::string> filtered;
  for (const std::string &filter : filters) {
    for (const std::string &path : paths) {
      if (NameMatches(path, filter)) {
        filtered.push_back(path);
      }
    }
    if (!filtered.empty()) {
      break;
    }
  }
  if (filtered.empty()) {
    filtered = paths;
  }
  std::string best;
  int64_t best_size = -1;
  for (const std::string &path : filtered) {
    const int64_t size = FileUtils::FileSize(path);
    if (size > best_size) {
      best_size = size;
      best = path;
    }
  }
  return best;
}

inline std::vector<std::string> ImagesInDirectory(const std::string &dir) {
  std::vector<std::string> images;
  if (dir.empty() || !FileUtils::IsDirectory(dir)) {
    return images;
  }
  for (const std::string &entry : FileUtils::ListDirectory(dir)) {
    if (FileUtils::IsFile(entry) && IsImageFile(entry)) {
      images.push_back(entry);
    }
  }
  return images;
}

inline std::string ArtForDirectory(const std::string &dir, const std::vector<std::string> &filters) {
  return PickBestArt(ImagesInDirectory(dir), filters);
}

}  // namespace CollectionDirectoryArt

#endif  // STRAWBERRY_COLLECTIONDIRECTORYART_H
