#ifndef STRAWBERRY_PLAYLISTMANAGERINTERFACE_H
#define STRAWBERRY_PLAYLISTMANAGERINTERFACE_H

#include "core/song.h"

#include <string>
#include <vector>

class CollectionBackend;
class Playlist;
class PlaylistBackend;
class PlaylistParser;

class PlaylistManagerInterface {
 public:
  virtual ~PlaylistManagerInterface() = default;

  virtual int current_id() const = 0;
  virtual int active_id() const = 0;
  virtual std::vector<int> playlist_ids() const = 0;
  virtual std::string playlist_name(int id) const = 0;
  virtual Playlist *playlist(int id) const = 0;
  virtual Playlist *current() const = 0;
  virtual Playlist *active() const = 0;
  virtual std::vector<Playlist *> GetAllPlaylists() const = 0;
  virtual void RemoveDeletedSongs() = 0;
  virtual std::string GetPlaylistName(int id) const = 0;

  virtual CollectionBackend *collection_backend() const = 0;
  virtual PlaylistBackend *playlist_backend() const = 0;

  virtual Playlist *New(const std::string &name = "Playlist", const SongList &songs = {}) = 0;
  virtual void Load(const std::string &filename) = 0;
  virtual void Save(int id, const std::string &filename) = 0;
  virtual void Rename(int id, const std::string &new_name) = 0;
  virtual void Favorite(int id, bool favorite) = 0;
  virtual void Delete(int id) = 0;
  virtual bool Close(int id) = 0;
  virtual void Open(int id) = 0;
  virtual void ChangePlaylistOrder(const std::vector<int> &ids) = 0;

  virtual void SetCurrentPlaylist(const std::string &name) = 0;
  virtual void SetCurrentPlaylist(int id) = 0;
  virtual void SetActivePlaylist(int id) = 0;
  virtual void SetActiveToCurrent() = 0;

  virtual void AppendSongs(const SongList &songs) = 0;
  virtual void InsertSongs(int id, const SongList &songs, int pos = -1) = 0;
  virtual void RemoveCurrentSong() = 0;
  virtual Song current_song() const = 0;
  virtual int current_row() const = 0;

  virtual void ClearCurrent() = 0;
  virtual void ShuffleCurrent() = 0;
  virtual void RemoveDuplicatesCurrent() = 0;
  virtual void RemoveUnavailableCurrent() = 0;
  virtual void SongChangeRequestProcessed(const std::string &url, bool valid) = 0;
  virtual void RateCurrentSong(float rating) = 0;
  virtual void RateCurrentSong2(int rating) = 0;
  virtual void PlaySmartPlaylist(const std::string &name, bool as_new, bool clear) = 0;
  virtual void SetActivePlaying() = 0;
  virtual void SetActivePaused() = 0;
  virtual void SetActiveStopped() = 0;
};

#endif
