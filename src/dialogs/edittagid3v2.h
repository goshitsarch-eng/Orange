#ifndef STRAWBERRY_EDITTAGID3V2_H
#define STRAWBERRY_EDITTAGID3V2_H

#include "core/song.h"
#include "tagreader/tagid3v2version.h"

#include <vector>

namespace EditTagId3v2 {

inline constexpr int kVersion3 = 3;
inline constexpr int kVersion4 = 4;
inline constexpr int kComboIndex3 = 0;
inline constexpr int kComboIndex4 = 1;

inline bool AnySupported(const SongList &songs) {
  for (const Song &song : songs) {
    if (song.id3v2_tags_supported()) {
      return true;
    }
  }
  return false;
}

inline int VersionForSongs(const SongList &songs) {
  int version = 0;
  for (const Song &song : songs) {
    if (!song.id3v2_tags_supported()) {
      continue;
    }
    if (song.id3v2_version() != kVersion3 && song.id3v2_version() != kVersion4) {
      return kVersion4;
    }
    if (version == 0) {
      version = song.id3v2_version();
    } else if (version != song.id3v2_version()) {
      return kVersion4;
    }
  }
  return version == 0 ? kVersion4 : version;
}

inline int ComboIndex(int version) { return version == kVersion3 ? kComboIndex3 : kComboIndex4; }

inline TagID3v2Version TagVersionFromIndex(int index) {
  return index == kComboIndex3 ? TagID3v2Version::V3 : TagID3v2Version::V4;
}

}  // namespace EditTagId3v2

#endif
