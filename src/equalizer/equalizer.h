#ifndef STRAWBERRY_EQUALIZER_H
#define STRAWBERRY_EQUALIZER_H

#include "core/signal.h"
#include "equalizer/equalizerpersist.h"

#include <algorithm>
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
  static int ClampBalance(int value) { return EqualizerPersist::ClampBalance(value); }
  const std::string &selected_preset() const { return selected_preset_; }
  bool stereo_balancer_enabled() const { return stereo_balancer_enabled_; }
  void set_stereo_balancer_enabled(bool enabled);
  int stereo_balance() const { return stereo_balance_; }
  void set_stereo_balance(int balance);
  float EffectiveBalanceFraction() const {
    return EqualizerPersist::EffectiveBalanceFraction(stereo_balancer_enabled_, stereo_balance_);
  }
  std::vector<int> EffectiveGains() const { return EqualizerPersist::EffectiveGains(enabled_, gains_); }
  int EffectivePreamp() const { return EqualizerPersist::EffectivePreamp(enabled_, preamp_); }
  Signal<bool, int, std::vector<int>> ParametersChanged;
  Signal<float> StereoBalanceChanged;

 private:
  void LoadBuiltinPresets();
  std::vector<int> ScaleTenths(const std::vector<int> &tenths) const;

  bool enabled_ = false;
  int preamp_ = 0;
  std::vector<int> gains_;
  std::map<std::string, std::vector<int>> presets_;
  std::vector<std::string> user_names_;
  std::string selected_preset_ = EqualizerPersist::kDefaultPreset;
  bool stereo_balancer_enabled_ = false;
  int stereo_balance_ = 0;
};

#endif
