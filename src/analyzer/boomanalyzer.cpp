#include "analyzer/boomanalyzer.h"

#include <algorithm>

void BoomAnalyzer::Advance(int width, int height, const std::vector<float> &bands) {
  (void)width;
  state_.Advance(bands, height);
}

void BoomAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  if (state_.bar_height.empty()) {
    DrawBars(cr, width, height, bands, false, false, false);
    return;
  }
  const size_t n = state_.bar_height.size();
  const double bar_w = n == 0 ? 0.0 : static_cast<double>(width) / static_cast<double>(n);
  for (size_t i = 0; i < n; ++i) {
    const double h = std::max(0.0, state_.bar_height[i]);
    const double y = static_cast<double>(height) - h;
    cairo_pattern_t *grad = cairo_pattern_create_linear(0, y, 0, height);
    cairo_pattern_add_color_stop_rgb(grad, 0.0, 1.0, 1.0, 1.0);
    cairo_pattern_add_color_stop_rgb(grad, 1.0, 0.10, 0.10, 0.25);
    cairo_set_source(cr, grad);
    cairo_rectangle(cr, static_cast<double>(i) * bar_w, y, bar_w - 1.0, h);
    cairo_fill(cr);
    cairo_pattern_destroy(grad);
    cairo_set_source_rgb(cr, 0.75, 0.75, 0.80);
    const double peak_y = static_cast<double>(height) - state_.peak_height[i];
    cairo_move_to(cr, static_cast<double>(i) * bar_w, peak_y);
    cairo_line_to(cr, static_cast<double>(i) * bar_w + bar_w - 1.0, peak_y);
    cairo_stroke(cr);
  }
}
