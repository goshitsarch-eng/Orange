#ifndef STRAWBERRY_PLAYLISTMANAGER_H
#define STRAWBERRY_PLAYLISTMANAGER_H

#include "core/signal.h"
#include "core/song.h"
#include "playlist/playlist.h"
#include "playlist/playlistbackend.h"
#include "playlist/playlistmanagerinterface.h"

#include <memory>
#include <string>
#include <vector>

class CollectionBackend;
class TaskManager;
class TagReader;
class UrlHandlers;

class PlaylistManager : public PlaylistManagerInterface {
 public:
  PlaylistManager(TaskManager *task_manager, TagReader *tagreader, UrlHandlers *url_handlers, PlaylistBackend *backend,
                  CollectionBackend *collection_backend);

  void Init();

  CollectionBackend *collection_backend() const override { return collection_backend_; }
  PlaylistBackend *playlist_backend() const override { return backend_; }

  int current_id() const override;
  int active_id() const override;
  std::vector<int> playlist_ids() const override;
  std::string playlist_name(int id) const override;
  Playlist *playlist(int id) const override;
  Playlist *current() const override { return current_; }
  Playlist *active() const override { return active_ ? active_ : current_; }
  std::vector<Playlist *> GetAllPlaylists() const override;
  void RemoveDeletedSongs() override;
  std::string GetPlaylistName(int id) const override { return playlist_name(id); }
  const std::vector<std::unique_ptr<Playlist>> &playlists() const { return playlists_; }
  std::vector<std::string> playlist_names() const;

  Playlist *New(const std::string &name = "Playlist", const SongList &songs = {}) override;
  void Load(const std::string &filename) override;
  void Save(int id, const std::string &filename) override;
  void Rename(int id, const std::string &new_name) override;
  void Favorite(int id, bool favorite) override;
  void Delete(int id) override;
  bool Close(int id) override;
  void Open(int id) override;
  void ChangePlaylistOrder(const std::vector<int> &ids) override;
  void SetCurrentPlaylist(const std::string &name) override;
  void SetCurrentPlaylist(int id) override;
  void SetActivePlaylist(int id) override;
  void SetActiveToCurrent() override;
  void SetCurrentRow(int row);
  int current_row() const override;
  Song current_song() const override;
  Song PeekNextSong() const;
  void Next();
  void Previous();
  void AppendSongs(const SongList &songs) override;
  void InsertSongs(int id, const SongList &songs, int pos = -1) override;
  void InsertUrls(const std::vector<std::string> &urls, int row = -1);
  void RemoveCurrentSong() override;
  void SaveActive();
  void SaveCurrent();
  void LoadAll();
  void RefillDynamic();
  void ExpandDynamic();
  void RepopulateDynamic();
  void TurnOffDynamic();
  void ClearCurrent() override;
  void ShuffleCurrent() override;
  void RemoveDuplicatesCurrent() override;
  void RemoveUnavailableCurrent() override;
  void RateCurrentSong(float rating) override;
  void RateCurrentSong2(int rating) override;
  void PlaySmartPlaylist(const std::string &name, bool as_new, bool clear) override;
  void SetActivePlaying() override;
  void SetActivePaused() override;
  void SetActiveStopped() override;
  void CycleRepeatMode();
  void CycleShuffleMode();

  Signal<Playlist *> PlaylistAdded;
  Signal<int> PlaylistClosed;
  Signal<int> PlaylistDeleted;
  Signal<int, std::string> PlaylistRenamed;
  Signal<int, bool> PlaylistFavorited;
  Signal<Playlist *> CurrentChanged;
  Signal<Playlist *> ActiveChanged;
  Signal<> PlaylistsLoaded;
  Signal<> SequenceChanged;

 private:
  Playlist *FindByName(const std::string &name) const;
  Playlist *FindById(int id) const;
  Playlist *Visible() const { return current_ ? current_ : active_; }
  Playlist *Playing() const { return active_ ? active_ : current_; }
  void Persist(Playlist *playlist);

  TaskManager *task_manager_;
  TagReader *tagreader_;
  UrlHandlers *url_handlers_;
  PlaylistBackend *backend_;
  CollectionBackend *collection_backend_;
  std::vector<std::unique_ptr<Playlist>> playlists_;
  Playlist *current_ = nullptr;
  Playlist *active_ = nullptr;
  int next_id_ = 1;
};

#endif  // STRAWBERRY_PLAYLISTMANAGER_H
