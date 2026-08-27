#include "analyzer/sonogramanalyzer.h"

void SonogramAnalyzer::Advance(int width, int height, const std::vector<float> &bands) {
  canvas_.EnsureSize(width, height);
  canvas_.ShiftLeft();
  canvas_.WriteRightColumn(bands);
}

void SonogramAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  canvas_.EnsureSize(width, height);
  (void)bands;
  for (int y = 0; y < canvas_.height; ++y) {
    for (int x = 0; x < canvas_.width; ++x) {
      const uint32_t pixel = canvas_.At(x, y);
      cairo_set_source_rgb(cr, ((pixel >> 16) & 0xFF) / 255.0, ((pixel >> 8) & 0xFF) / 255.0, (pixel & 0xFF) / 255.0);
      cairo_rectangle(cr, x, y, 1, 1);
      cairo_fill(cr);
    }
  }
}
