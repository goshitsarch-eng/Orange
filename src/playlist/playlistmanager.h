#ifndef STRAWBERRY_PLAYLISTMANAGER_H
#define STRAWBERRY_PLAYLISTMANAGER_H

#include "core/signal.h"
#include "core/song.h"
#include "playlist/playlist.h"
#include "playlist/playlistbackend.h"

#include <memory>
#include <string>
#include <vector>

class CollectionBackend;
class TaskManager;
class TagReader;
class UrlHandlers;

class PlaylistManager {
 public:
  PlaylistManager(TaskManager *task_manager, TagReader *tagreader, UrlHandlers *url_handlers, PlaylistBackend *backend,
                  CollectionBackend *collection_backend);

  void Init();
  Playlist *active() const { return active_; }
  Playlist *current() const { return active_; }
  const std::vector<std::unique_ptr<Playlist>> &playlists() const { return playlists_; }

  Playlist *New(const std::string &name = "Playlist");
  void Close(int id);
  void SetCurrentPlaylist(const std::string &name);
  void SetCurrentRow(int row);
  int current_row() const;
  Song current_song() const;
  void Next();
  void Previous();
  void AppendSongs(const SongList &songs);
  void InsertUrls(const std::vector<std::string> &urls, int row = -1);
  void SaveActive();
  void LoadAll();

  Signal<Playlist *> PlaylistAdded;
  Signal<int> PlaylistClosed;
  Signal<Playlist *> CurrentChanged;
  Signal<> PlaylistsLoaded;

 private:
  Playlist *FindByName(const std::string &name) const;
  Playlist *FindById(int id) const;

  TaskManager *task_manager_;
  TagReader *tagreader_;
  UrlHandlers *url_handlers_;
  PlaylistBackend *backend_;
  CollectionBackend *collection_backend_;
  std::vector<std::unique_ptr<Playlist>> playlists_;
  Playlist *active_ = nullptr;
};

#endif  // STRAWBERRY_PLAYLISTMANAGER_H
