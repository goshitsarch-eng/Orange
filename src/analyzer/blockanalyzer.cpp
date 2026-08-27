#include "analyzer/blockanalyzer.h"

#include <algorithm>

void BlockAnalyzer::Advance(int width, int height, const std::vector<float> &bands) {
  (void)width;
  state_.Advance(bands, height);
}

void BlockAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  if (state_.store.empty()) {
    DrawBars(cr, width, height, bands, false, false, true);
    return;
  }
  const size_t n = state_.store.size();
  const double cell_w = n == 0 ? 0.0 : static_cast<double>(width) / static_cast<double>(n);
  const double block_h = BlockAnalyzerState::kHeight;
  const double gap = 1.0;
  cairo_set_source_rgb(cr, 0.12, 0.12, 0.12);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);
  for (size_t x = 0; x < n; ++x) {
    const double left = static_cast<double>(x) * cell_w;
    if (state_.fade_intensity[x] > 0) {
      const double fade = static_cast<double>(state_.fade_intensity[x]) / BlockAnalyzerState::kFadeSize;
      cairo_set_source_rgba(cr, 0.15, 0.45, 0.28, fade);
      const int fade_row = state_.fade_pos[x];
      cairo_rectangle(cr, left, state_.y_offset + fade_row * (block_h + gap), cell_w - 1.0, (state_.rows - fade_row) * (block_h + gap));
      cairo_fill(cr);
    }
    const int y = static_cast<int>(state_.store[x]);
    cairo_set_source_rgb(cr, 0.2, 0.8, 0.4);
    for (int row = y; row < state_.rows; ++row) {
      cairo_rectangle(cr, left, state_.y_offset + row * (block_h + gap), cell_w - 1.0, block_h);
      cairo_fill(cr);
    }
    cairo_set_source_rgb(cr, 0.55, 1.0, 0.70);
    cairo_rectangle(cr, left, state_.y_offset + y * (block_h + gap), cell_w - 1.0, block_h);
    cairo_fill(cr);
  }
}
