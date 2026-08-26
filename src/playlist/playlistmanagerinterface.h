#ifndef STRAWBERRY_PLAYLISTMANAGERINTERFACE_H
#define STRAWBERRY_PLAYLISTMANAGERINTERFACE_H

#include "core/song.h"

#include <string>
#include <vector>

class Playlist;

class PlaylistManagerInterface {
 public:
  virtual ~PlaylistManagerInterface() = default;

  virtual Playlist *active() const = 0;
  virtual Playlist *current() const = 0;
  virtual Playlist *New(const std::string &name = "Playlist") = 0;
  virtual void SetCurrentPlaylist(const std::string &name) = 0;
  virtual void AppendSongs(const SongList &songs) = 0;
  virtual Song current_song() const = 0;
  virtual int current_row() const = 0;
};

#endif
