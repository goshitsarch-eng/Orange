#include "waveform/waveformrenderer.h"

#include <algorithm>

void WaveformRenderer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &peaks) {
  if (peaks.empty() || width <= 0 || height <= 0) {
    return;
  }
  cairo_set_source_rgb(cr, 0.23, 0.63, 0.95);
  const double mid = height / 2.0;
  for (int x = 0; x < width; ++x) {
    const size_t i = peaks.size() * static_cast<size_t>(x) / static_cast<size_t>(width);
    const double h = std::clamp(peaks[i], 0.0f, 1.0f) * mid;
    cairo_move_to(cr, x + 0.5, mid - h);
    cairo_line_to(cr, x + 0.5, mid + h);
    cairo_stroke(cr);
  }
}
