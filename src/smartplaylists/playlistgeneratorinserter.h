#ifndef STRAWBERRY_PLAYLISTGENERATORINSERTER_H
#define STRAWBERRY_PLAYLISTGENERATORINSERTER_H

#include "core/song.h"
#include "smartplaylists/playlistgenerator.h"

#include <memory>
#include <string>

class Playlist;

class PlaylistGeneratorInserter {
 public:
  SongList Load(const std::shared_ptr<PlaylistGenerator> &generator) const;
  int Insert(Playlist *playlist, const std::shared_ptr<PlaylistGenerator> &generator, int row = -1) const;
  int InsertMore(Playlist *playlist, const std::shared_ptr<PlaylistGenerator> &generator, int count) const;
};

#endif
