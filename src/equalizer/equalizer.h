#ifndef STRAWBERRY_EQUALIZER_H
#define STRAWBERRY_EQUALIZER_H

#include "core/signal.h"

#include <map>
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
  bool SavePreset(const std::string &name);
  bool DeletePreset(const std::string &name);
  bool IsBuiltin(const std::string &name) const;
  std::vector<std::string> Presets() const;
  static std::vector<std::string> BuiltinPresetNames();
  Signal<bool, int, std::vector<int>> ParametersChanged;

 private:
  void LoadBuiltinPresets();
  std::vector<int> ScaleTenths(const std::vector<int> &tenths) const;

  bool enabled_ = false;
  int preamp_ = 0;
  std::vector<int> gains_;
  std::map<std::string, std::vector<int>> presets_;
  std::vector<std::string> user_names_;
};

#endif
