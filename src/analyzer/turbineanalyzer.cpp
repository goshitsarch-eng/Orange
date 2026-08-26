#include "analyzer/turbineanalyzer.h"

#include <algorithm>

void TurbineAnalyzer::Advance(int width, int height, const std::vector<float> &bands) {
  (void)width;
  const double max_h = std::max(0.0, static_cast<double>(height) / 2.0 - 1.0);
  state_.Advance(bands, height, 0.5, max_h);
}

void TurbineAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  if (state_.bar_height.empty()) {
    DrawBars(cr, width, height, bands, false, true, false);
    return;
  }
  const size_t n = state_.bar_height.size();
  const double bar_w = n == 0 ? 0.0 : static_cast<double>(width) / static_cast<double>(n);
  const double mid = static_cast<double>(height) / 2.0;
  for (size_t i = 0; i < n; ++i) {
    const double h = std::max(0.0, state_.bar_height[i]);
    cairo_set_source_rgb(cr, 0.9, 0.45 + std::min(h / std::max(1.0, mid), 1.0) * 0.4, 0.1);
    cairo_rectangle(cr, static_cast<double>(i) * bar_w, mid - h, bar_w - 1.0, h * 2.0);
    cairo_fill(cr);
    cairo_set_source_rgb(cr, 0.75, 0.75, 0.80);
    const double peak = state_.peak_height[i];
    cairo_move_to(cr, static_cast<double>(i) * bar_w, mid - peak);
    cairo_line_to(cr, static_cast<double>(i) * bar_w + bar_w - 1.0, mid - peak);
    cairo_move_to(cr, static_cast<double>(i) * bar_w, mid + peak);
    cairo_line_to(cr, static_cast<double>(i) * bar_w + bar_w - 1.0, mid + peak);
    cairo_stroke(cr);
  }
}
