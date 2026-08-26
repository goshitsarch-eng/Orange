#ifndef STRAWBERRY_SCROBBLEMETADATA_H
#define STRAWBERRY_SCROBBLEMETADATA_H

#include "core/song.h"

#include <cstdint>
#include <string>

struct ScrobbleMetadata {
  std::string artist;
  std::string album;
  std::string title;
  std::string albumartist;
  int track = 0;
  int64_t length_nanosec = 0;
  uint64_t timestamp = 0;

  static std::string StripRemasteredTitle(const std::string &title);
  static ScrobbleMetadata FromSong(const Song &song, uint64_t timestamp = 0, bool prefer_album_artist = false,
                                  bool strip_remastered = false);
  static ScrobbleMetadata FromSongSettings(const Song &song, uint64_t timestamp = 0);
};

#endif
