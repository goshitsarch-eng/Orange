#ifndef STRAWBERRY_PLAYLISTMANAGER_H
#define STRAWBERRY_PLAYLISTMANAGER_H

#include "core/signal.h"
#include "core/song.h"
#include "playlist/playlist.h"
#include "playlist/playlistbackend.h"
#include "playlist/playlistmanagerinterface.h"
#include "playlist/playlistsaveschedule.h"

#include <glib.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

class CollectionBackend;
class NetworkAccessManager;
class TaskManager;
class TagReader;
class TagReaderClient;
class UrlHandlers;

class PlaylistManager : public PlaylistManagerInterface {
 public:
  PlaylistManager(TaskManager *task_manager, TagReader *tagreader, UrlHandlers *url_handlers, PlaylistBackend *backend,
                  CollectionBackend *collection_backend);
  ~PlaylistManager();

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
  void SetPlaylistUiPath(int id, const std::string &path);
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
  void MoveRowsBetween(int source_id, int dest_id, const std::vector<int> &rows, int dest_pos = -1);
  bool UndoCrossMove(int source_id, int dest_id);
  void InsertUrls(const std::vector<std::string> &urls, int row = -1, bool play_now = false, bool enqueue = false,
                  bool enqueue_next = false);
  void LoadAudioCD(int row = -1, bool play_now = false, bool enqueue = false, bool enqueue_next = false,
                   const std::vector<std::string> &cdda_fallbacks = {});
  void set_network(NetworkAccessManager *network) { network_ = network; }
  void RemoveCurrentSong() override;
  void SaveActive();
  void SaveCurrent();
  void FlushPendingSaves();
  void LoadAll();
  bool playlists_loaded() const { return playlists_loaded_; }
  void RefillDynamic();
  void ExpandDynamic();
  void RepopulateDynamic();
  void UpdateCollectionSongs(const SongList &songs);
  void set_tagreader_client(TagReaderClient *client);
  void ArmTagReaderPump();
  bool PumpTagReader();
  void TurnOffDynamic();
  void ClearCurrent() override;
  void ShuffleCurrent() override;
  void RemoveDuplicatesCurrent() override;
  void RemoveUnavailableCurrent() override;
  void SongChangeRequestProcessed(const std::string &url, bool valid) override;
  void RateCurrentSong(float rating) override;
  void RateCurrentSong2(int rating) override;
  void PlaySmartPlaylist(const std::string &name, bool as_new, bool clear) override;
  void SetActivePlaying() override;
  void SetActivePaused() override;
  void SetActiveStopped() override;
  void CycleRepeatMode();
  void CycleShuffleMode();

  Signal<Playlist *> PlaylistAdded;
  Signal<Playlist *> PlaylistChanged;
  Signal<int> PlaylistClosed;
  Signal<int> PlaylistDeleted;
  Signal<int, std::string> PlaylistRenamed;
  Signal<int, bool> PlaylistFavorited;
  Signal<Playlist *> CurrentChanged;
  Signal<Playlist *> ActiveChanged;
  Signal<> PlaylistsLoaded;
  Signal<> SequenceChanged;
  Signal<std::string> Error;
  Signal<int> PlayRequested;

 private:
  Playlist *FindByName(const std::string &name) const;
  Playlist *FindById(int id) const;
  Playlist *Visible() const { return current_ ? current_ : active_; }
  Playlist *Playing() const { return active_ ? active_ : current_; }
  void Persist(Playlist *playlist);
  void PersistNow(Playlist *playlist);
  void PersistLastPlayed(Playlist *playlist);
  void SchedulePersist(Playlist *playlist, PlaylistSaveSchedule::Intent intent);
  void ArmSaveTimer();
  void WatchSaves(Playlist *playlist);

  TaskManager *task_manager_;
  TagReader *tagreader_;
  UrlHandlers *url_handlers_;
  PlaylistBackend *backend_;
  CollectionBackend *collection_backend_;
  NetworkAccessManager *network_ = nullptr;
  TagReaderClient *tagreader_client_ = nullptr;
  std::vector<std::unique_ptr<Playlist>> playlists_;
  Playlist *current_ = nullptr;
  Playlist *active_ = nullptr;
  int next_id_ = 1;
  std::set<int> pending_ids_;
  PlaylistSaveSchedule::Intent pending_intent_ = PlaylistSaveSchedule::Intent::None;
  guint save_timeout_id_ = 0;
  guint tag_pump_id_ = 0;
  bool playlists_loaded_ = false;
};

#endif  // STRAWBERRY_PLAYLISTMANAGER_H
