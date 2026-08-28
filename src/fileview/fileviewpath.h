#ifndef STRAWBERRY_FILEVIEWPATH_H
#define STRAWBERRY_FILEVIEWPATH_H

#include <string>

namespace FileViewPath {

inline std::string DefaultHome(const char *music_dir, const char *home_dir, const char *fallback = ".") {
  if (music_dir && *music_dir) {
    return music_dir;
  }
  if (home_dir && *home_dir) {
    return home_dir;
  }
  return fallback ? fallback : ".";
}

// Qt MainWindow restores MainWindowSettings::kFilePath when it is an existing directory.
inline std::string Initial(const std::string &saved, const std::string &fallback, bool saved_is_directory) {
  if (!saved.empty() && saved_is_directory) {
    return saved;
  }
  return fallback;
}

}  // namespace FileViewPath

#endif
