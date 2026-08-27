#ifndef STRAWBERRY_FILEVIEWURLS_H
#define STRAWBERRY_FILEVIEWURLS_H

#include "utilities/fileutils.h"

#include <string>
#include <vector>

namespace FileViewUrls {

// Qt File View MimeData keeps selected folders as file:// URLs; SongLoader walks them off the UI thread.
inline std::string FromPath(const std::string &path) { return path.empty() ? std::string() : FileUtils::UriFromPath(path); }

inline std::vector<std::string> FromPaths(const std::vector<std::string> &paths) {
  std::vector<std::string> urls;
  urls.reserve(paths.size());
  for (const std::string &path : paths) {
    if (path.empty()) {
      continue;
    }
    urls.push_back(FileUtils::UriFromPath(path));
  }
  return urls;
}

}  // namespace FileViewUrls

#endif
