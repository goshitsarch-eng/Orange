#ifndef STRAWBERRY_ENGINEMETADATA_H
#define STRAWBERRY_ENGINEMETADATA_H

#include "core/song.h"

#include <cstdint>
#include <string>

struct EngineMetadata {
  enum class Type { Any, Current, Next };

  Type type = Type::Any;
  std::string media_url;
  std::string stream_url;
  std::string title;
  std::string artist;
  std::string album;
  std::string comment;
  std::string genre;
  std::string lyrics;
  int year = 0;
  int track = 0;
  int64_t length_nanosec = 0;
  Song::FileType filetype = Song::FileType::Unknown;
  int bitrate = 0;
  int samplerate = 0;
  int bitdepth = 0;

  Song ToSong(Song::Source source = Song::Source::Unknown) const;
};

#endif
