#include "equalizer/equalizer.h"

#include "core/settings.h"
#include "equalizer/equalizergain.h"
#include "equalizer/equalizerpersist.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace {

std::vector<int> ParseGains(const std::string &text) {
  std::vector<int> gains(EqualizerGain::kBandCount, 0);
  std::istringstream stream(text);
  std::string part;
  size_t i = 0;
  while (i < gains.size() && std::getline(stream, part, ',')) {
    gains[i++] = EqualizerGain::ClampSlider(std::atoi(part.c_str()));
  }
  return gains;
}

std::string JoinGains(const std::vector<int> &gains) {
  std::string text;
  for (size_t i = 0; i < gains.size(); ++i) {
    if (i > 0) {
      text += ",";
    }
    text += std::to_string(gains[i]);
  }
  return text;
}

}  // namespace

Equalizer::Equalizer() : gains_(EqualizerGain::kBandCount, 0) { ReloadSettings(); }

void Equalizer::LoadBuiltinPresets() {
  presets_["Custom"] = std::vector<int>(EqualizerGain::kBandCount, 0);
  presets_["Classical"] = {0, 0, 0, 0, 0, 0, -40, -40, -40, -50};
  presets_["Club"] = {0, 0, 20, 30, 30, 30, 20, 0, 0, 0};
  presets_["Dance"] = {50, 35, 10, 0, 0, -30, -40, -40, 0, 0};
  presets_["Full Bass"] = {70, 70, 70, 40, 20, -45, -50, -55, -55, -55};
  presets_["Full Treble"] = {-50, -50, -50, -25, 15, 55, 80, 80, 80, 85};
  presets_["Full Bass + Treble"] = {35, 30, 0, -40, -25, 10, 45, 55, 60, 60};
  presets_["Laptop/Headphones"] = {25, 50, 25, -20, 0, -30, -40, -40, 0, 0};
  presets_["Large Hall"] = {50, 50, 30, 30, 0, -25, -25, -25, 0, 0};
  presets_["Live"] = {-25, 0, 20, 25, 30, 30, 20, 15, 15, 10};
  presets_["Party"] = {35, 35, 0, 0, 0, 0, 0, 0, 35, 35};
  presets_["Pop"] = {-10, 25, 35, 40, 25, -5, -15, -15, -10, -10};
  presets_["Reggae"] = {0, 0, -5, -30, 0, -35, -35, 0, 0, 0};
  presets_["Rock"] = {40, 25, -30, -40, -20, 20, 45, 55, 55, 55};
  presets_["Soft"] = {25, 10, -5, -15, -5, 20, 45, 50, 55, 60};
  presets_["Ska"] = {-15, -25, -25, -5, 20, 30, 45, 50, 55, 50};
  presets_["Soft Rock"] = {20, 20, 10, -5, -25, -30, -20, -5, 15, 45};
  presets_["Techno"] = {40, 30, 0, -30, -25, 0, 40, 50, 50, 45};
  presets_["Zero"] = std::vector<int>(EqualizerGain::kBandCount, 0);
}

std::vector<std::string> Equalizer::BuiltinPresetNames() {
  return {"Custom",
          "Classical",
          "Club",
          "Dance",
          "Full Bass",
          "Full Treble",
          "Full Bass + Treble",
          "Laptop/Headphones",
          "Large Hall",
          "Live",
          "Party",
          "Pop",
          "Reggae",
          "Rock",
          "Soft",
          "Ska",
          "Soft Rock",
          "Techno",
          "Zero"};
}

void Equalizer::ReloadSettings() {
  Settings s;
  s.BeginGroup(EqualizerPersist::kSettingsGroup);
  enabled_ = s.BoolValue("enabled", false);
  preamp_ = s.IntValue("preamp", 0);
  selected_preset_ = EqualizerPersist::PresetOrDefault(s.Value(EqualizerPersist::kSelectedPreset, EqualizerPersist::kDefaultPreset));
  for (int i = 0; i < EqualizerGain::kBandCount; ++i) {
    if (static_cast<int>(gains_.size()) < EqualizerGain::kBandCount) {
      gains_.resize(EqualizerGain::kBandCount, 0);
    }
    gains_[i] = EqualizerGain::ClampSlider(s.IntValue("band" + std::to_string(i), 0));
  }
  LoadBuiltinPresets();
  user_names_.clear();
  for (const std::string &entry : StrUtils::Split(s.Value("user_presets"), '|')) {
    const auto colon = entry.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    const std::string name = entry.substr(0, colon);
    presets_[name] = ParseGains(entry.substr(colon + 1));
    user_names_.push_back(name);
  }
  Settings backend;
  backend.BeginGroup("Backend");
  const int legacy_balance = backend.IntValue("stereobalance", 0);
  stereo_balance_ = EqualizerPersist::ClampBalance(
      s.Contains(EqualizerPersist::kStereoBalance) ? s.IntValue(EqualizerPersist::kStereoBalance, 0) : legacy_balance);
  stereo_balancer_enabled_ = EqualizerPersist::MigrateBalancerEnabled(s.Contains(EqualizerPersist::kEnableStereoBalancer),
                                                                      s.BoolValue(EqualizerPersist::kEnableStereoBalancer, false),
                                                                      stereo_balance_);
}

void Equalizer::Save() {
  Settings s;
  s.BeginGroup(EqualizerPersist::kSettingsGroup);
  s.SetBoolValue("enabled", enabled_);
  s.SetIntValue("preamp", preamp_);
  s.SetValue(EqualizerPersist::kSelectedPreset, selected_preset_);
  s.SetBoolValue(EqualizerPersist::kEnableStereoBalancer, stereo_balancer_enabled_);
  s.SetIntValue(EqualizerPersist::kStereoBalance, stereo_balance_);
  for (int i = 0; i < EqualizerGain::kBandCount; ++i) {
    s.SetIntValue("band" + std::to_string(i), gains_[i]);
  }
  std::string blob;
  for (size_t i = 0; i < user_names_.size(); ++i) {
    if (i > 0) {
      blob += "|";
    }
    blob += user_names_[i] + ":" + JoinGains(presets_[user_names_[i]]);
  }
  s.SetValue("user_presets", blob);
  s.BeginGroup("Backend");
  s.SetIntValue("stereobalance", EqualizerPersist::EffectiveBalance(stereo_balancer_enabled_, stereo_balance_));
  s.Sync();
}

void Equalizer::set_enabled(bool enabled) {
  enabled_ = enabled;
  Save();
  ParametersChanged.Emit(enabled_, preamp_, gains_);
}

void Equalizer::set_preamp(int preamp) {
  preamp_ = preamp;
  Save();
  ParametersChanged.Emit(enabled_, preamp_, gains_);
}

void Equalizer::set_gain(int band, int gain) {
  if (band >= 0 && band < EqualizerGain::kBandCount) {
    gains_[band] = EqualizerGain::ClampSlider(gain);
    Save();
    ParametersChanged.Emit(enabled_, preamp_, gains_);
  }
}

void Equalizer::set_stereo_balancer_enabled(bool enabled) {
  stereo_balancer_enabled_ = enabled;
  Save();
  StereoBalanceChanged.Emit(EffectiveBalanceFraction());
}

void Equalizer::set_stereo_balance(int balance) {
  stereo_balance_ = EqualizerPersist::ClampBalance(balance);
  Save();
  StereoBalanceChanged.Emit(EffectiveBalanceFraction());
}

void Equalizer::LoadPreset(const std::string &name) {
  const auto it = presets_.find(name);
  if (it == presets_.end()) {
    return;
  }
  gains_ = it->second;
  selected_preset_ = name;
  Save();
  ParametersChanged.Emit(enabled_, preamp_, gains_);
}

bool Equalizer::HasPreset(const std::string &name) const { return presets_.find(name) != presets_.end(); }

bool Equalizer::MatchesPreset(const std::string &name) const {
  const auto it = presets_.find(name);
  return it != presets_.end() && EqualizerPersist::GainsMatch(gains_, it->second);
}

bool Equalizer::IsBuiltin(const std::string &name) const {
  const auto names = BuiltinPresetNames();
  return std::find(names.begin(), names.end(), name) != names.end();
}

bool Equalizer::SavePreset(const std::string &name) {
  if (name.empty() || IsBuiltin(name)) {
    return false;
  }
  presets_[name] = gains_;
  if (std::find(user_names_.begin(), user_names_.end(), name) == user_names_.end()) {
    user_names_.push_back(name);
  }
  Save();
  return true;
}

bool Equalizer::DeletePreset(const std::string &name) {
  if (IsBuiltin(name)) {
    return false;
  }
  presets_.erase(name);
  user_names_.erase(std::remove(user_names_.begin(), user_names_.end(), name), user_names_.end());
  Save();
  return true;
}

std::vector<std::string> Equalizer::Presets() const {
  std::vector<std::string> names = BuiltinPresetNames();
  names.insert(names.end(), user_names_.begin(), user_names_.end());
  return names;
}
