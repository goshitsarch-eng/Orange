#ifndef STRAWBERRY_FILEVIEWICONS_H
#define STRAWBERRY_FILEVIEWICONS_H

#include "constants/filefilterconstants.h"

#include <string>

namespace FileViewIcons {

inline const char *FolderIcon() { return "folder-symbolic"; }
inline const char *AudioIcon() { return "audio-x-generic-symbolic"; }
inline const char *PlaylistIcon() { return "view-list-symbolic"; }
inline const char *OtherIcon() { return "text-x-generic-symbolic"; }

inline const char *IconName(bool is_directory, const std::string &path) {
  if (is_directory) {
    return FolderIcon();
  }
  if (FileFilterConstants::PathMatchesGlobs(path, FileFilterConstants::kPlaylist)) {
    return PlaylistIcon();
  }
  if (FileFilterConstants::PathMatchesGlobs(path, FileFilterConstants::kAudio)) {
    return AudioIcon();
  }
  return OtherIcon();
}

}  // namespace FileViewIcons

#endif
