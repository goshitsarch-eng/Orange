#include "analyzer/analyzerbase.h"

#include <algorithm>
#include <cmath>

double AnalyzerBase::BarWidth(int width, const std::vector<float> &bands) {
  return bands.empty() ? 0.0 : static_cast<double>(width) / static_cast<double>(bands.size());
}

void AnalyzerBase::DrawWave(cairo_t *cr, int width, int height, const std::vector<float> &bands, double r, double g, double b) {
  const double bar_w = BarWidth(width, bands);
  cairo_set_source_rgb(cr, r, g, b);
  cairo_move_to(cr, 0, height / 2.0);
  for (size_t i = 0; i < bands.size(); ++i) {
    cairo_line_to(cr, static_cast<double>(i) * bar_w, height / 2.0 - static_cast<double>(bands[i]) * height / 2.0);
  }
  cairo_stroke(cr);
}

void AnalyzerBase::DrawBars(cairo_t *cr, int width, int height, const std::vector<float> &bands, bool rainbow, bool turbine, bool blocks) {
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
