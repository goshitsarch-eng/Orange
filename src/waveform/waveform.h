#ifndef STRAWBERRY_WAVEFORM_H
#define STRAWBERRY_WAVEFORM_H
#include "core/song.h"
#include "core/signal.h"
#include <vector>
class WaveformLoader {
 public:
  std::vector<float> Load(const Song &song);
};
class WaveformController {
 public:
  explicit WaveformController(WaveformLoader *loader);
  void Load(const Song &song);
  const std::vector<float> &data() const { return data_; }
  Signal<std::vector<float>> Ready;
 private:
  WaveformLoader *loader_;
  std::vector<float> data_;
};
#endif
