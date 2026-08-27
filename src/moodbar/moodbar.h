#ifndef STRAWBERRY_MOODBAR_H
#define STRAWBERRY_MOODBAR_H

#include "core/signal.h"
#include "core/song.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class MoodbarLoader {
 public:
  std::vector<uint8_t> Load(const Song &song);
  std::vector<uint8_t> LoadCached(const Song &song) const;
  std::vector<uint8_t> Generate(const Song &song, bool save, const std::string &cache_dir) const;
};

class MoodbarController {
 public:
  explicit MoodbarController(MoodbarLoader *loader);
  ~MoodbarController();
  void Load(const Song &song);
  void ReloadSettings();
  void CurrentSongChanged(const Song &song);
  void PlaybackStopped();
  void SetEnabled(bool enabled);
  void ApplyGenerated(int generation, std::vector<uint8_t> data);
  void ApplyGenerated(int generation, std::vector<uint8_t> data, const std::string &url);
  const std::vector<uint8_t> &data() const { return data_; }
  const Song &current_song() const { return current_song_; }
  bool enabled() const { return enabled_; }
  bool playback_active() const { return playback_active_; }
  bool busy() const { return busy_; }
  int generation() const { return generation_; }
  Signal<std::vector<uint8_t>> Ready;

 private:
  void ApplyEnabledTransition(bool was_enabled);
  void Generate(const Song &song);

  MoodbarLoader *loader_;
  Song current_song_;
  std::vector<uint8_t> data_;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  int generation_ = 0;
  bool busy_ = false;
  bool enabled_ = false;
  bool playback_active_ = false;
};

#endif
