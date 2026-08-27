#ifndef STRAWBERRY_MOODBARPATHS_H
#define STRAWBERRY_MOODBARPATHS_H

#include "utilities/fileutils.h"

#include <string>
#include <vector>

namespace MoodbarPaths {

inline std::string Stem(const std::string &path) {
  const std::string name = FileUtils::BaseName(path);
  const auto dot = name.rfind('.');
  if (dot == std::string::npos || dot == 0) {
    return name;
  }
  return name.substr(0, dot);
}

inline std::string HiddenSidecar(const std::string &song_path) {
  return FileUtils::Join(FileUtils::DirName(song_path), "." + Stem(song_path) + ".mood");
}

inline std::string VisibleSidecar(const std::string &song_path) {
  return FileUtils::Join(FileUtils::DirName(song_path), Stem(song_path) + ".mood");
}

inline std::vector<std::string> Sidecars(const std::string &song_path) {
  if (song_path.empty()) {
    return {};
  }
  return {HiddenSidecar(song_path), VisibleSidecar(song_path)};
}

inline std::string CacheFile(const std::string &cache_dir, const std::string &url) {
  return FileUtils::Join(cache_dir, FileUtils::BaseName(url) + ".mood");
}

}  // namespace MoodbarPaths

#endif
