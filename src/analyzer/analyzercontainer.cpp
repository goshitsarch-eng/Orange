#include "analyzer/analyzercontainer.h"

#include "analyzer/blockanalyzer.h"
#include "analyzer/boomanalyzer.h"
#include "analyzer/rainbowanalyzer.h"
#include "analyzer/sonogramanalyzer.h"
#include "analyzer/turbineanalyzer.h"
#include "analyzer/waverubberanalyzer.h"

AnalyzerContainer::AnalyzerContainer() { current_ = Create("Bar"); }

void AnalyzerContainer::set_type(const std::string &type) { current_ = Create(type); }

std::string AnalyzerContainer::type() const { return current_ ? current_->name() : std::string(); }

void AnalyzerContainer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  if (current_) {
    current_->Draw(cr, width, height, bands);
  }
}

std::unique_ptr<AnalyzerBase> AnalyzerContainer::Create(const std::string &type) {
  if (type == "Rainbow") return std::make_unique<RainbowAnalyzer>();
  if (type == "Turbine") return std::make_unique<TurbineAnalyzer>();
  if (type == "Wave") return std::make_unique<WaveRubberAnalyzer>();
  if (type == "Sonic") return std::make_unique<SonogramAnalyzer>();
  if (type == "Block") return std::make_unique<BlockAnalyzer>();
  return std::make_unique<BoomAnalyzer>();
}
