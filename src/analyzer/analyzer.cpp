#include "analyzer/analyzer.h"

#include "utilities/audioanalysis.h"

#include <cmath>

Analyzer::Analyzer() { container_.set_type(type_); }

void Analyzer::set_type(const std::string &type) {
  type_ = type;
  container_.set_type(type);
}

void Analyzer::Draw(cairo_t *cr, int width, int height) const { container_.Draw(cr, width, height, bands_); }

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

std::vector<std::string> Analyzer::Types() { return {"Bar", "Rainbow", "Turbine", "Wave", "Sonic", "Block"}; }
