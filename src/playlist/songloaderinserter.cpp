#include "playlist/songloaderinserter.h"

#include "core/songloader.h"
#include "playlist/playlist.h"

SongLoaderInserter::SongLoaderInserter(TagReader *tagreader) : tagreader_(tagreader) {}

SongList SongLoaderInserter::Load(const std::vector<std::string> &urls) const {
  if (!tagreader_) {
    return {};
  }
  SongLoader loader(nullptr, nullptr, tagreader_);
  const SongLoader::Result result = loader.LoadMany(urls);
  if (result == SongLoader::Result::BlockingLoadRequired) {
    loader.LoadFilenamesBlocking();
  }
  loader.LoadMetadataBlocking();
  return loader.songs();
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
