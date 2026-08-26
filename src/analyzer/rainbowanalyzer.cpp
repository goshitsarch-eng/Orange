#include "analyzer/rainbowanalyzer.h"

#include <algorithm>

RainbowAnalyzer::RainbowAnalyzer(Style style) : style_(style) {
  if (style_ == Style::Dash) {
    name_ = "RainbowDash";
  } else if (style_ == Style::Nyan) {
    name_ = "NyanCat";
  }
}

void RainbowAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  if (style_ == Style::Nyan) {
    const double bar_w = BarWidth(width, bands);
    for (size_t i = 0; i < bands.size(); ++i) {
      const double h = std::max(1.0, static_cast<double>(bands[i]) * height);
      cairo_set_source_rgb(cr, 1.0, 0.4 + 0.4 * (static_cast<double>(i) / std::max<size_t>(1, bands.size())), 0.7);
      cairo_rectangle(cr, static_cast<double>(i) * bar_w, height - h, bar_w - 1.0, h);
      cairo_fill(cr);
    }
    return;
  }
  DrawBars(cr, width, height, bands, true, style_ == Style::Dash, false);
}
