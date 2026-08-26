#ifndef STRAWBERRY_EQUALIZER_H
#define STRAWBERRY_EQUALIZER_H
#include "core/signal.h"
#include <string>
#include <vector>
class Equalizer {
 public:
  Equalizer();
  void ReloadSettings();
  void Save();
  bool enabled() const { return enabled_; }
  void set_enabled(bool enabled);
  int preamp() const { return preamp_; }
  void set_preamp(int preamp);
  const std::vector<int> &gains() const { return gains_; }
  void set_gain(int band, int gain);
  void LoadPreset(const std::string &name);
  std::vector<std::string> Presets() const;
  Signal<bool, int, std::vector<int>> ParametersChanged;
 private:
  bool enabled_ = false;
  int preamp_ = 0;
  std::vector<int> gains_;
};
#endif
