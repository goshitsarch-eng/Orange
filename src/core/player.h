#ifndef STRAWBERRY_PLAYER_H
#define STRAWBERRY_PLAYER_H

#include "constants/behavioursettings.h"
#include "core/playerinterface.h"
#include "core/signal.h"
#include "core/song.h"
#include "core/urlhandler.h"
#include "engine/gstengine.h"

#include <memory>
#include <string>
#include <vector>

class PlaylistManager;
class Queue;
class TaskManager;
class UrlHandlers;

class Player : public PlayerInterface {
 public:
  Player(TaskManager *task_manager, UrlHandlers *url_handlers, PlaylistManager *playlist_manager);
  ~Player();

  void Init();
  void SetQueue(Queue *queue) { queue_ = queue; }
  GstEngine *engine() const { return engine_.get(); }
  TaskManager *task_manager() const { return task_manager_; }
  EngineBase::State GetState() const override;
  unsigned GetVolume() const override { return volume_; }
  const Song &current_song() const override { return current_song_; }

  void Play() override;
  void Play(uint64_t offset_nanosec);
  void PlayPause() override;
  void Pause() override;
  void Stop(bool stop_after = false) override;
  void StopAfterCurrent();
  bool stop_after_current() const { return stop_after_current_; }
  void set_stop_after_current(bool stop) { stop_after_current_ = stop; }
  void Next() override;
  void Previous() override;
  void RestartOrPrevious();
  void SeekTo(int64_t seconds) override;
  void Seek(int64_t nanosec);
  void SeekForward();
  void SeekBackward();
  void SetVolume(unsigned volume) override;
  void SetVolumeFromEngine(unsigned volume);
  void VolumeUp();
  void VolumeDown();
  void Mute();
  void PlayAt(int index, bool pause = false, uint64_t offset_nanosec = 0);
  void PlayPlaylist(const std::string &name);
  void ReloadSettings();
  void LoadVolume();
  void SaveVolume();
  void SavePlaybackStatus();
  void ResumePlayback();
  void PlaylistsLoaded();
  void ShowOSD();
  void TogglePrettyOSD();
  void SyncCurrentMetadata(const Song &updated);

  Signal<Song> SongChanged;
  Signal<Song> NowPlayingRefresh;
  Signal<unsigned> VolumeChanged;
  Signal<EngineBase::State> StateChanged;
  Signal<int64_t, int64_t> PositionChanged;
  Signal<Song, bool> ForceShowOSD;
  Signal<Song, int64_t> PlaybackFinished;
  Signal<Song> TrackEndedPlaycount;
  Signal<Song, int64_t, int64_t> TrackSkipped;
  Signal<> Paused;
  Signal<> Playing;
  Signal<> Stopped;
  Signal<> PlaylistFinished;
  Signal<std::string> Error;

 private:
  void HandleEngineState(EngineBase::State state);
  void HandleTrackEnded();
  void StartFromPlaylist(uint64_t offset_nanosec);
  void PlayQueueHead(int track_change_flags = GstEngine::Manual);
  void PreloadNext();
  void PlayCurrent(bool pause, uint64_t offset_nanosec = 0, int track_change_flags = GstEngine::Manual);
  void PlayLoadedSong(bool pause, int track_change_flags = GstEngine::Manual, uint64_t offset_nanosec = 0);
  void FinishCurrentPlayback();
  void Advance(int track_change_flags);
  void MaybeEmitTrackSkipped();
  int SameAlbumFlags(int track_change_flags, const Song &from, const Song &to) const;
  void CancelIntroTimeout();
  void ArmIntroTimeout();

  TaskManager *task_manager_;
  UrlHandlers *url_handlers_;
  PlaylistManager *playlist_manager_;
  Queue *queue_ = nullptr;
  std::unique_ptr<GstEngine> engine_;
  Song current_song_;
  unsigned volume_ = 100;
  unsigned volume_before_mute_ = 100;
  bool stop_after_current_ = false;
  bool preloaded_ = false;
  bool finished_current_ = true;
  int seek_step_sec_ = 10;
  unsigned volume_increment_ = 5;
  bool continue_on_error_ = false;
  bool greyout_ = true;
  int error_count_ = 0;
  unsigned intro_timeout_id_ = 0;
  int intro_generation_ = 0;

  void HandleEngineError(const std::string &error);
  void HandleFatalError();
  void HandleInvalidSongRequested(const std::string &url);
  void HandleValidSongRequested(const std::string &url);
  void HandleEngineMetadata(const Song &song);
  void HandleLoadResult(const UrlHandler::LoadResult &result);
  void StartEnginePlayback(bool pause, int track_change_flags, uint64_t offset_nanosec);
  void UnPause();

  std::vector<std::string> loading_async_;
  bool pending_play_pause_ = false;
  int pending_play_flags_ = 0;
  uint64_t pending_play_offset_ = 0;
  int64_t pause_started_sec_ = 0;
  int64_t last_previous_press_sec_ = 0;
  BehaviourSettings::PreviousBehaviour menu_previous_mode_ = BehaviourSettings::kDefaultMenuPreviousMode;
  bool playlists_loaded_ = false;
  bool play_requested_ = false;
};

#endif  // STRAWBERRY_PLAYER_H
