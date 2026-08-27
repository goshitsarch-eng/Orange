#ifndef STRAWBERRY_ORGANIZE_H
#define STRAWBERRY_ORGANIZE_H

#include "core/musicstorage.h"
#include "core/song.h"
#include "organize/organizeformat.h"

#include <string>
#include <vector>

class Organize {
 public:
  struct Error {
    std::string song;
    std::string message;
  };

  struct Options {
    bool move = false;
    bool overwrite = false;
    bool albumcover = false;
    MusicStorage::TranscodeMode transcode_mode = MusicStorage::TranscodeMode::Transcode_Never;
    Song::FileType transcode_format = Song::FileType::Unknown;
    std::vector<Song::FileType> supported_filetypes;
    class CollectionBackend *collection_backend = nullptr;
    class TagReader *tagreader = nullptr;
    int collection_directory_id = -1;
    bool destination_is_collection = false;
    std::string cover_cache_path;
  };

  std::vector<Error> Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, bool move);
  std::vector<Error> Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, const Options &options);
  static std::string CoverPathForSong(const Song &song);
};

#endif
