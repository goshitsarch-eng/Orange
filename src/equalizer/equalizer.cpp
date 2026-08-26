#include "equalizer/equalizer.h"
#include "core/settings.h"
Equalizer::Equalizer() : gains_(10, 0) { ReloadSettings(); }
void Equalizer::ReloadSettings() {
  Settings s; s.BeginGroup("Equalizer");
  enabled_ = s.BoolValue("enabled", false);
  preamp_ = s.IntValue("preamp", 0);
  for (int i = 0; i < 10; ++i) gains_[i] = s.IntValue("band" + std::to_string(i), 0);
}
void Equalizer::Save() {
  Settings s; s.BeginGroup("Equalizer");
  s.SetBoolValue("enabled", enabled_); s.SetIntValue("preamp", preamp_);
  for (int i = 0; i < 10; ++i) s.SetIntValue("band" + std::to_string(i), gains_[i]);
  s.Sync();
}
void Equalizer::set_enabled(bool enabled) { enabled_ = enabled; ParametersChanged.Emit(enabled_, preamp_, gains_); }
void Equalizer::set_preamp(int preamp) { preamp_ = preamp; ParametersChanged.Emit(enabled_, preamp_, gains_); }
void Equalizer::set_gain(int band, int gain) { if (band>=0 && band<10) { gains_[band]=gain; ParametersChanged.Emit(enabled_, preamp_, gains_); } }
void Equalizer::LoadPreset(const std::string &name) {
  gains_.assign(10, 0);
  if (name == "Rock") { gains_ = {4,3,0,-2,-1,1,3,4,4,4}; }
  else if (name == "Pop") { gains_ = {-1,0,2,3,2,0,-1,-1,0,1}; }
  else if (name == "Jazz") { gains_ = {2,1,0,1,2,2,0,1,2,3}; }
  else if (name == "Classical") { gains_ = {3,2,0,0,0,0,-1,-2,-2,-3}; }
  else if (name == "Bass") { gains_ = {6,5,4,2,0,-1,-2,-2,-2,-2}; }
  ParametersChanged.Emit(enabled_, preamp_, gains_);
}
std::vector<std::string> Equalizer::Presets() const { return {"Flat","Rock","Pop","Jazz","Classical","Bass"}; }
