#ifndef STRAWBERRY_PLAYER_H
#define STRAWBERRY_PLAYER_H

#include "core/signal.h"
#include "core/song.h"
#include "engine/gstengine.h"

#include <memory>
#include <string>
#include <vector>

class PlaylistManager;
class TaskManager;
class UrlHandlers;

class Player {
 public:
  Player(TaskManager *task_manager, UrlHandlers *url_handlers, PlaylistManager *playlist_manager);

  void Init();
  GstEngine *engine() const { return engine_.get(); }
  GstEngine::State GetState() const;
  unsigned GetVolume() const { return volume_; }
  const Song &current_song() const { return current_song_; }

  void Play();
  void PlayPause();
  void Pause();
  void Stop(bool stop_after = false);
  void StopAfterCurrent();
  void Next();
  void Previous();
  void RestartOrPrevious();
  void SeekTo(int64_t seconds);
  void SeekForward();
  void SeekBackward();
  void SetVolume(unsigned volume);
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
  Signal<GstEngine::State> StateChanged;
  Signal<int64_t, int64_t> PositionChanged;
  Signal<Song> ForceShowOSD;
  Signal<> Paused;
  Signal<> Playing;
  Signal<> Stopped;

 private:
  void HandleEngineState(GstEngine::State state);
  void HandleTrackEnded();
  void PlayCurrent(bool pause);

  TaskManager *task_manager_;
  UrlHandlers *url_handlers_;
  PlaylistManager *playlist_manager_;
  std::unique_ptr<GstEngine> engine_;
  Song current_song_;
  unsigned volume_ = 100;
  unsigned volume_before_mute_ = 100;
  bool stop_after_current_ = false;
  int seek_step_sec_ = 10;
  unsigned volume_increment_ = 5;
};

#endif  // STRAWBERRY_PLAYER_H
