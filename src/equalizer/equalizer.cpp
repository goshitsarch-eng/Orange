#include "equalizer/equalizer.h"

#include "core/settings.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace {

std::vector<int> ParseGains(const std::string &text) {
  std::vector<int> gains(10, 0);
  std::istringstream stream(text);
  std::string part;
  size_t i = 0;
  while (i < gains.size() && std::getline(stream, part, ',')) {
    gains[i++] = std::atoi(part.c_str());
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

Equalizer::Equalizer() : gains_(10, 0) { ReloadSettings(); }

std::vector<int> Equalizer::ScaleTenths(const std::vector<int> &tenths) const {
  std::vector<int> gains(10, 0);
  for (size_t i = 0; i < tenths.size() && i < gains.size(); ++i) {
    gains[i] = std::clamp(tenths[i] / 10, -24, 12);
  }
  return gains;
}

void Equalizer::LoadBuiltinPresets() {
  presets_["Custom"] = std::vector<int>(10, 0);
  presets_["Classical"] = ScaleTenths({0, 0, 0, 0, 0, 0, -40, -40, -40, -50});
  presets_["Club"] = ScaleTenths({0, 0, 20, 30, 30, 30, 20, 0, 0, 0});
  presets_["Dance"] = ScaleTenths({50, 35, 10, 0, 0, -30, -40, -40, 0, 0});
  presets_["Full Bass"] = ScaleTenths({70, 70, 70, 40, 20, -45, -50, -55, -55, -55});
  presets_["Full Treble"] = ScaleTenths({-50, -50, -50, -25, 15, 55, 80, 80, 80, 85});
  presets_["Full Bass + Treble"] = ScaleTenths({35, 30, 0, -40, -25, 10, 45, 55, 60, 60});
  presets_["Laptop/Headphones"] = ScaleTenths({25, 50, 25, -20, 0, -30, -40, -40, 0, 0});
  presets_["Large Hall"] = ScaleTenths({50, 50, 30, 30, 0, -25, -25, -25, 0, 0});
  presets_["Live"] = ScaleTenths({-25, 0, 20, 25, 30, 30, 20, 15, 15, 10});
  presets_["Party"] = ScaleTenths({35, 35, 0, 0, 0, 0, 0, 0, 35, 35});
  presets_["Pop"] = ScaleTenths({-10, 25, 35, 40, 25, -5, -15, -15, -10, -10});
  presets_["Reggae"] = ScaleTenths({0, 0, -5, -30, 0, -35, -35, 0, 0, 0});
  presets_["Rock"] = ScaleTenths({40, 25, -30, -40, -20, 20, 45, 55, 55, 55});
  presets_["Soft"] = ScaleTenths({25, 10, -5, -15, -5, 20, 45, 50, 55, 60});
  presets_["Ska"] = ScaleTenths({-15, -25, -25, -5, 20, 30, 45, 50, 55, 50});
  presets_["Soft Rock"] = ScaleTenths({20, 20, 10, -5, -25, -30, -20, -5, 15, 45});
  presets_["Techno"] = ScaleTenths({40, 30, 0, -30, -25, 0, 40, 50, 50, 45});
  presets_["Zero"] = std::vector<int>(10, 0);
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
  s.BeginGroup("Equalizer");
  enabled_ = s.BoolValue("enabled", false);
  preamp_ = s.IntValue("preamp", 0);
  for (int i = 0; i < 10; ++i) {
    if (static_cast<int>(gains_.size()) < 10) {
      gains_.resize(10, 0);
    }
    gains_[i] = s.IntValue("band" + std::to_string(i), 0);
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
}

void Equalizer::Save() {
  Settings s;
  s.BeginGroup("Equalizer");
  s.SetBoolValue("enabled", enabled_);
  s.SetIntValue("preamp", preamp_);
  for (int i = 0; i < 10; ++i) {
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
  s.Sync();
}

void Equalizer::set_enabled(bool enabled) {
  enabled_ = enabled;
  ParametersChanged.Emit(enabled_, preamp_, gains_);
}

void Equalizer::set_preamp(int preamp) {
  preamp_ = preamp;
  ParametersChanged.Emit(enabled_, preamp_, gains_);
}

void Equalizer::set_gain(int band, int gain) {
  if (band >= 0 && band < 10) {
    gains_[band] = gain;
    ParametersChanged.Emit(enabled_, preamp_, gains_);
  }
}

void Equalizer::LoadPreset(const std::string &name) {
  const auto it = presets_.find(name);
  if (it == presets_.end()) {
    return;
  }
  gains_ = it->second;
  ParametersChanged.Emit(enabled_, preamp_, gains_);
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
