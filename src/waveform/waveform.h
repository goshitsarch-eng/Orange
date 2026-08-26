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
};

class WaveformController {
 public:
  explicit WaveformController(WaveformLoader *loader);
  ~WaveformController();
  void Load(const Song &song);
  void ApplyGenerated(int generation, std::vector<float> data);
  const std::vector<float> &data() const { return data_; }
  bool busy() const { return busy_; }
  int generation() const { return generation_; }
  Signal<std::vector<float>> Ready;

 private:
  WaveformLoader *loader_;
  std::vector<float> data_;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  int generation_ = 0;
  bool busy_ = false;
};

#endif
