#ifndef STRAWBERRY_WAVEFORM_H
#define STRAWBERRY_WAVEFORM_H

#include "core/signal.h"
#include "core/song.h"

#include <memory>
#include <string>
#include <vector>

class WaveformLoader {
 public:
  std::vector<float> Load(const Song &song);
  std::vector<float> LoadCached(const Song &song) const;
  std::vector<float> Generate(const Song &song, bool save, const std::string &cache_dir) const;
  void WriteSidecar(const std::string &url, const std::vector<float> &peaks) const;
};

class WaveformController {
 public:
  explicit WaveformController(WaveformLoader *loader);
  ~WaveformController();
  void Load(const Song &song);
  void ReloadSettings();
  void CurrentSongChanged(const Song &song);
  void PlaybackStopped();
  void SetEnabled(bool enabled);
  void ApplyGenerated(int generation, std::vector<float> data);
  void ApplyGenerated(int generation, std::vector<float> data, const std::string &url);
  const std::vector<float> &data() const { return data_; }
  const Song &current_song() const { return current_song_; }
  bool enabled() const { return enabled_; }
  bool save() const { return save_; }
  bool playback_active() const { return playback_active_; }
  bool busy() const { return busy_; }
  int generation() const { return generation_; }
  Signal<std::vector<float>> Ready;

 private:
  void ApplyEnabledTransition(bool was_enabled);
  void Generate(const Song &song);

  WaveformLoader *loader_;
  Song current_song_;
  std::vector<float> data_;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  int generation_ = 0;
  bool busy_ = false;
  bool enabled_ = false;
  bool save_ = false;
  bool playback_active_ = false;
};

#endif
