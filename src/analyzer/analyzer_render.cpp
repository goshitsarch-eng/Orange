#include "analyzer/blockanalyzer.h"
#include "analyzer/boomanalyzer.h"
#include "analyzer/rainbowanalyzer.h"
#include "analyzer/sonogramanalyzer.h"
#include "analyzer/turbineanalyzer.h"
#include "analyzer/waverubberanalyzer.h"
#include "analyzer/analyzercontainer.h"

#include <algorithm>
#include <cmath>

namespace {

double BarWidth(int width, const std::vector<float> &bands) {
  return bands.empty() ? 0.0 : static_cast<double>(width) / static_cast<double>(bands.size());
}

void DrawWave(cairo_t *cr, int width, int height, const std::vector<float> &bands, double r, double g, double b) {
  const double bar_w = BarWidth(width, bands);
  cairo_set_source_rgb(cr, r, g, b);
  cairo_move_to(cr, 0, height / 2.0);
  for (size_t i = 0; i < bands.size(); ++i) {
    cairo_line_to(cr, static_cast<double>(i) * bar_w, height / 2.0 - static_cast<double>(bands[i]) * height / 2.0);
  }
  cairo_stroke(cr);
}

void DrawBars(cairo_t *cr, int width, int height, const std::vector<float> &bands, bool rainbow, bool turbine, bool blocks) {
  const double bar_w = BarWidth(width, bands);
  for (size_t i = 0; i < bands.size(); ++i) {
    const double h = std::max(1.0, static_cast<double>(bands[i]) * height);
    if (rainbow) {
      cairo_set_source_rgb(cr, static_cast<double>(i) / bands.size(), 0.4, 1.0 - static_cast<double>(i) / bands.size());
    } else if (turbine) {
      cairo_set_source_rgb(cr, 0.9, 0.45 + bands[i] * 0.4, 0.1);
    } else if (blocks) {
      cairo_set_source_rgb(cr, 0.2, 0.8, 0.4);
    } else {
      cairo_set_source_rgb(cr, 0.23, 0.63, 0.95);
    }
    if (blocks) {
      const int count = std::max(1, static_cast<int>(h / 4));
      for (int b = 0; b < count; ++b) {
        cairo_rectangle(cr, static_cast<double>(i) * bar_w, height - (b + 1) * 4, bar_w - 1.0, 3);
        cairo_fill(cr);
      }
    } else {
      cairo_rectangle(cr, static_cast<double>(i) * bar_w, height - h, bar_w - 1.0, h);
      cairo_fill(cr);
    }
  }
}

}  // namespace

void BlockAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawBars(cr, width, height, bands, false, false, true);
}

void BoomAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawBars(cr, width, height, bands, false, false, false);
}

void RainbowAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawBars(cr, width, height, bands, true, false, false);
}

void TurbineAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawBars(cr, width, height, bands, false, true, false);
}

void SonogramAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawWave(cr, width, height, bands, 0.95, 0.63, 0.35);
}

void WaveRubberAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawWave(cr, width, height, bands, 0.23, 0.63, 0.95);
}

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
