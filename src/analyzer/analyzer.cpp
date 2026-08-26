#include "analyzer/analyzer.h"

#include "utilities/audioanalysis.h"

#include <cmath>

void Analyzer::SetMagnitudes(const std::vector<float> &db) { SetEngineScope(AudioAnalysis::ScopeFromMagnitudes(db)); }

void Analyzer::SetEngineScope(const std::vector<int16_t> &scope) {
  if (scope.empty()) return;
  const size_t n = bands_.size();
  const size_t chunk = std::max<size_t>(1, scope.size() / n);
  for (size_t i = 0; i < n; ++i) {
    double sum = 0;
    for (size_t j = 0; j < chunk && i * chunk + j < scope.size(); ++j) sum += std::abs(scope[i * chunk + j]);
    bands_[i] = static_cast<float>(sum / (chunk * 32768.0));
  }
}
std::vector<std::string> Analyzer::Types() { return {"Bar","Rainbow","Turbine","Wave","Sonic","Block"}; }
