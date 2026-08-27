#include "smartplaylists/playlistgeneratorinserter.h"

#include "playlist/playlist.h"

SongList PlaylistGeneratorInserter::Load(const std::shared_ptr<PlaylistGenerator> &generator) const {
  if (!generator) {
    return {};
  }
  return generator->Generate();
}

int PlaylistGeneratorInserter::Insert(Playlist *playlist, const std::shared_ptr<PlaylistGenerator> &generator, int row) const {
  const SongList songs = Load(generator);
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

int PlaylistGeneratorInserter::InsertMore(Playlist *playlist, const std::shared_ptr<PlaylistGenerator> &generator, int count) const {
  if (!playlist || !generator) {
    return 0;
  }
  const SongList songs = generator->GenerateMore(count);
  if (songs.empty()) {
    return 0;
  }
  playlist->AppendSongs(songs);
  return static_cast<int>(songs.size());
}
