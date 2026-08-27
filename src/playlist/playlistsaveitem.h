#ifndef STRAWBERRY_PLAYLISTSAVEITEM_H
#define STRAWBERRY_PLAYLISTSAVEITEM_H

#include "core/song.h"

namespace PlaylistSaveItem {

inline unsigned long long Begin(unsigned long long previous) { return previous + 1; }

inline bool ShouldApply(unsigned long long started, unsigned long long current) { return started == current; }

inline bool ShouldWriteFile(const Song &song) { return song.IsEditable(); }

inline Song ChooseMetadata(bool read_ok, const Song &from_file, const Song &fallback) { return read_ok && from_file.is_valid() ? from_file : fallback; }

inline std::string WriteError(const std::string &filename, const std::string &error) {
  if (error.empty()) {
    return "Could not write metadata to " + filename;
  }
  return "Could not write metadata to " + filename + ": " + error;
}

}  // namespace PlaylistSaveItem

#endif  // STRAWBERRY_PLAYLISTSAVEITEM_H
