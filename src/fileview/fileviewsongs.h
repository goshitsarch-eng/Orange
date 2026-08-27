#ifndef STRAWBERRY_FILEVIEWSONGS_H
#define STRAWBERRY_FILEVIEWSONGS_H

#include "core/song.h"
#include "utilities/fileutils.h"

#include <functional>
#include <string>
#include <vector>

class FileViewSongs {
 public:
  using ReadFile = std::function<Song(const std::string &)>;

  static Song FromPath(const std::string &path) {
    Song song(Song::Source::LocalFile);
    song.set_valid(true);
    song.set_url(FileUtils::UriFromPath(path));
    song.set_basefilename(FileUtils::BaseName(path));
    song.set_title(FileUtils::BaseName(path));
    return song;
  }

  static SongList FromPaths(const std::vector<std::string> &paths, const ReadFile &read_file = {}) {
    SongList songs;
    songs.reserve(paths.size());
    for (const std::string &path : paths) {
      if (path.empty() || FileUtils::IsDirectory(path)) {
        continue;
      }
      if (read_file) {
        Song song = read_file(path);
        if (!song.is_valid()) {
          song = FromPath(path);
        }
        songs.push_back(song);
        continue;
      }
      songs.push_back(FromPath(path));
    }
    return songs;
  }
};

#endif
