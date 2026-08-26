#ifndef STRAWBERRY_SAVEPLAYLISTSOPTIONS_H
#define STRAWBERRY_SAVEPLAYLISTSOPTIONS_H

#include "utilities/fileutils.h"

#include <string>
#include <vector>

namespace SavePlaylistsOptions {

inline const char *Title() { return "Select directory for saving playlists"; }
inline const char *BrowseTitle() { return "Select directory for the playlists"; }
inline const char *TypeLabel() { return "Type"; }
inline const char *DirectoryMissing() { return "Directory does not exist."; }
inline const char *DirectoryMissingTitle() { return DirectoryMissing(); }
inline const char *DirectoryMissingBody() { return DirectoryMissing(); }

inline std::string DefaultExtension(const std::string &stored) { return stored.empty() ? "m3u" : stored; }

inline std::vector<std::string> ExtensionChoices() { return {"m3u", "m3u8", "pls", "xspf", "asx"}; }

inline std::string DestFilename(const std::string &name, const std::string &ext) {
  std::string suffix = ext;
  if (!suffix.empty() && suffix.front() != '.') {
    suffix = "." + suffix;
  }
  return name + suffix;
}

inline bool ValidateDirectory(const std::string &path) { return !path.empty() && FileUtils::IsDirectory(path); }

inline std::string FallbackPath(const std::string &stored, const std::string &home) { return stored.empty() ? home : stored; }

inline int ExtensionIndex(const std::vector<std::string> &choices, const std::string &ext) {
  for (size_t i = 0; i < choices.size(); ++i) {
    if (choices[i] == ext) {
      return static_cast<int>(i);
    }
  }
  return 0;
}

}  // namespace SavePlaylistsOptions

#endif  // STRAWBERRY_SAVEPLAYLISTSOPTIONS_H
