#ifndef STRAWBERRY_GPODDELETE_H
#define STRAWBERRY_GPODDELETE_H

#include "utilities/fileutils.h"

#include <string>

namespace GPodDelete {

// Qt GPodDevice::RemoveTrackFromITunesDb: strip the mount prefix, then '/' → ':'.
inline std::string IpodPathFromLocal(const std::string &path, const std::string &mount_path) {
  std::string ipod = path;
  if (!mount_path.empty() && ipod.rfind(mount_path, 0) == 0) {
    const bool trailing_slash = mount_path.back() == '/';
    const size_t drop = trailing_slash ? mount_path.size() - 1 : mount_path.size();
    if (ipod.size() >= drop) {
      ipod = ipod.substr(drop);
    }
  }
  for (char &ch : ipod) {
    if (ch == '/') {
      ch = ':';
    }
  }
  return ipod;
}

inline std::string LocalPath(const std::string &url) {
  const std::string path = FileUtils::PathFromUri(url);
  return path.empty() ? url : path;
}

inline std::string IpodPathFromUrl(const std::string &url, const std::string &mount_path) {
  return IpodPathFromLocal(LocalPath(url), mount_path);
}

inline bool TrackMatches(const char *ipod_path, const std::string &wanted) { return ipod_path && wanted == ipod_path; }

}  // namespace GPodDelete

#endif
