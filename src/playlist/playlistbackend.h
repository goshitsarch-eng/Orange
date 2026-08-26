#ifndef STRAWBERRY_PLAYLISTBACKEND_H
#define STRAWBERRY_PLAYLISTBACKEND_H

#include "core/song.h"
#include "playlist/playlist.h"

#include <memory>
#include <string>
#include <vector>

class CollectionBackend;
class Database;
class TagReader;

struct PlaylistMetadata {
  int id = -1;
  std::string name;
  int last_played = -1;
  int ui_order = 0;
  std::string special_type;
  std::string ui_path;
  bool favorite = false;
};

class PlaylistBackend {
 public:
  PlaylistBackend(Database *database, TagReader *tagreader, CollectionBackend *collection_backend);

  std::vector<PlaylistMetadata> GetAllPlaylists();
  std::unique_ptr<Playlist> LoadPlaylist(int id);
  int SavePlaylist(Playlist *playlist);
  void DeletePlaylist(int id);
  void RenamePlaylist(int id, const std::string &name);
  void SetFavorite(int id, bool favorite);
  void SetPlaylistUiPath(int id, const std::string &path);

 private:
  Database *database_;
  TagReader *tagreader_;
  CollectionBackend *collection_backend_;
};

#endif  // STRAWBERRY_PLAYLISTBACKEND_H
