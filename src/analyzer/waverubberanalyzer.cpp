#include "analyzer/waverubberanalyzer.h"

#include "analyzer/waveamplitude.h"

void WaveRubberAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  if (bands.empty() || width <= 0 || height <= 0) {
    return;
  }
  const int mid_y = WaveAmplitude::MidY(height);
  const double x_scale = static_cast<double>(width) / static_cast<double>(bands.size());
  double prev_y = WaveAmplitude::DrawY(bands.front(), mid_y);
  cairo_set_line_width(cr, 1.5);
  for (size_t i = 0; i < bands.size(); ++i) {
    double r = 0, g = 0, b = 0;
    WaveAmplitude::Color(bands[i], &r, &g, &b);
    cairo_set_source_rgb(cr, r, g, b);
    const double x = static_cast<double>(i) * x_scale;
    const double y = WaveAmplitude::DrawY(bands[i], mid_y);
    cairo_move_to(cr, x, prev_y);
    cairo_line_to(cr, x + x_scale, y);
    cairo_stroke(cr);
    prev_y = y;
  }
}
