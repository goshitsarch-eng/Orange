#include "playlist/songloaderinserter.h"

#include "playlist/playlist.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

SongLoaderInserter::SongLoaderInserter(TagReader *tagreader) : tagreader_(tagreader) {}

SongList SongLoaderInserter::Load(const std::vector<std::string> &urls) const {
  SongList songs;
  if (!tagreader_) {
    return songs;
  }
  for (const std::string &url : urls) {
    const std::string path = FileUtils::PathFromUri(url);
    Song song = tagreader_->ReadFile(path.empty() ? url : path);
    if (!song.is_valid() && !url.empty()) {
      song.set_url(url);
      song.set_title(FileUtils::BaseName(path.empty() ? url : path));
      song.set_valid(true);
    }
    if (song.is_valid()) {
      songs.push_back(song);
    }
  }
  return songs;
}

int SongLoaderInserter::Insert(Playlist *playlist, const std::vector<std::string> &urls, int row) const {
  const SongList songs = Load(urls);
  if (!playlist || songs.empty()) {
    return 0;
  }
  if (row < 0) {
    playlist->AppendSongs(songs);
  } else {
    playlist->InsertSongs(row, songs);
  }
  return static_cast<int>(songs.size());
}
