#ifndef STRAWBERRY_PLAYER_H
#define STRAWBERRY_PLAYER_H

#include "core/playerinterface.h"
#include "core/signal.h"
#include "core/song.h"
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

  void Init();
  void SetQueue(Queue *queue) { queue_ = queue; }
  GstEngine *engine() const { return engine_.get(); }
  TaskManager *task_manager() const { return task_manager_; }
  EngineBase::State GetState() const override;
  unsigned GetVolume() const override { return volume_; }
  const Song &current_song() const override { return current_song_; }

  void Play() override;
  void PlayPause() override;
  void Pause() override;
  void Stop(bool stop_after = false) override;
  void StopAfterCurrent();
  void Next() override;
  void Previous() override;
  void RestartOrPrevious();
  void SeekTo(int64_t seconds) override;
  void Seek(int64_t nanosec);
  void SeekForward();
  void SeekBackward();
  void SetVolume(unsigned volume) override;
  void VolumeUp();
  void VolumeDown();
  void Mute();
  void PlayAt(int index, bool pause = false);
  void PlayPlaylist(const std::string &name);
  void ReloadSettings();
  void LoadVolume();
  void SaveVolume();
  void ShowOSD();

  Signal<Song> SongChanged;
  Signal<unsigned> VolumeChanged;
  Signal<EngineBase::State> StateChanged;
  Signal<int64_t, int64_t> PositionChanged;
  Signal<Song> ForceShowOSD;
  Signal<> Paused;
  Signal<> Playing;
  Signal<> Stopped;

 private:
  void HandleEngineState(EngineBase::State state);
  void HandleTrackEnded();
  void PreloadNext();
  void PlayCurrent(bool pause);
  void PlayLoadedSong(bool pause, int track_change_flags = GstEngine::Manual);

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
  int seek_step_sec_ = 10;
  unsigned volume_increment_ = 5;
};

#endif  // STRAWBERRY_PLAYER_H
