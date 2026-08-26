#include "analyzer/analyzer.h"

#include "constants/analyzersettings.h"
#include "core/settings.h"
#include "utilities/audioanalysis.h"

#include <algorithm>
#include <cmath>

Analyzer::Analyzer() {
  ReloadSettings();
  container_.set_type(type_);
}

void Analyzer::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(AnalyzerSettings::kSettingsGroup);
  type_ = settings.Value(AnalyzerSettings::kType, AnalyzerSettings::kDefaultType);
  enabled_ = settings.BoolValue(AnalyzerSettings::kEnabled, AnalyzerSettings::kDefaultEnabled);
  framerate_ = ClampFramerate(settings.IntValue(AnalyzerSettings::kFramerate, AnalyzerSettings::kDefaultFramerate));
  container_.set_type(type_);
}

void Analyzer::Save() const {
  Settings settings;
  settings.BeginGroup(AnalyzerSettings::kSettingsGroup);
  settings.SetValue(AnalyzerSettings::kType, type_);
  settings.SetBoolValue(AnalyzerSettings::kEnabled, enabled_);
  settings.SetIntValue(AnalyzerSettings::kFramerate, framerate_);
  settings.Sync();
}

void Analyzer::set_type(const std::string &type) {
  type_ = type;
  container_.set_type(type);
  Save();
}

void Analyzer::set_enabled(bool enabled) {
  enabled_ = enabled;
  Save();
}

void Analyzer::set_framerate(int fps) {
  framerate_ = ClampFramerate(fps);
  Save();
}

int Analyzer::ClampFramerate(int fps) {
  return std::clamp(fps, AnalyzerSettings::kMinFramerate, AnalyzerSettings::kMaxFramerate);
}

std::string Analyzer::NextType(const std::string &current) {
  const auto types = Types();
  auto it = std::find(types.begin(), types.end(), current);
  const size_t index = it == types.end() ? 0 : (static_cast<size_t>(std::distance(types.begin(), it)) + 1) % types.size();
  return types[index];
}

void Analyzer::Draw(cairo_t *cr, int width, int height) const {
  if (!enabled_) {
    return;
  }
  container_.Draw(cr, width, height, bands_);
}

void Analyzer::SetMagnitudes(const std::vector<float> &db) { SetEngineScope(AudioAnalysis::ScopeFromMagnitudes(db)); }

void Analyzer::SetEngineScope(const std::vector<int16_t> &scope) {
  if (scope.empty()) return;
  std::vector<float> windowed(scope.begin(), scope.end());
  for (float &sample : windowed) {
    sample /= 32768.0f;
  }
  fht_.Transform(&windowed);
  const size_t n = bands_.size();
  const size_t chunk = std::max<size_t>(1, windowed.size() / n);
  for (size_t i = 0; i < n; ++i) {
    double sum = 0;
    for (size_t j = 0; j < chunk && i * chunk + j < windowed.size(); ++j) sum += std::abs(windowed[i * chunk + j]);
    bands_[i] = static_cast<float>(sum / static_cast<double>(chunk));
  }
}

std::vector<std::string> Analyzer::Types() {
  return {"Bar", "Rainbow", "RainbowDash", "NyanCat", "Turbine", "Wave", "Sonic", "Block"};
}
