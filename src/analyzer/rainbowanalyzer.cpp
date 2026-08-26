#include "analyzer/rainbowanalyzer.h"

#include <algorithm>

RainbowAnalyzer::RainbowAnalyzer(Style style) : style_(style) {
  if (style_ == Style::Dash) {
    name_ = "RainbowDash";
  } else if (style_ == Style::Nyan) {
    name_ = "NyanCat";
  }
}

void RainbowAnalyzer::Advance(int width, int height, const std::vector<float> &bands) {
  (void)width;
  (void)height;
  canvas_.Advance(bands);
}

void RainbowAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  (void)bands;
  cairo_set_source_rgb(cr, 0.06, 0.26, 0.45);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);
  if (canvas_.history.empty()) {
    return;
  }
  const int rainbow_h = style_ == Style::Dash ? 16 : 21;
  const double top = static_cast<double>(height) / 2.0 - static_cast<double>(rainbow_h) / 2.0;
  const double x_scale = width <= 1 ? 1.0 : static_cast<double>(width - 1) / static_cast<double>(RainbowCanvas::kHistorySize - 1);
  cairo_set_line_width(cr, std::max(2.0, static_cast<double>(rainbow_h) / RainbowCanvas::kBands));
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  for (int band = RainbowCanvas::kBands - 1; band >= 0; --band) {
    double r = 0, g = 0, b = 0;
    RainbowCanvas::ColorForBand(band, &r, &g, &b);
    if (style_ == Style::Nyan) {
      r = 1.0;
      g = 0.4 + 0.4 * (static_cast<double>(band) / std::max(1, RainbowCanvas::kBands - 1));
      b = 0.7;
    }
    cairo_set_source_rgb(cr, r, g, b);
    const double y0 = static_cast<double>(rainbow_h) / static_cast<double>(RainbowCanvas::kBands + 1) * (static_cast<double>(band) + 0.5) + top;
    cairo_move_to(cr, 0, y0 + canvas_.At(band, 0) * RainbowCanvas::kPixelScale);
    for (int x = 1; x < RainbowCanvas::kHistorySize; ++x) {
      cairo_line_to(cr, static_cast<double>(x) * x_scale, y0 + canvas_.At(band, x) * RainbowCanvas::kPixelScale);
    }
    cairo_stroke(cr);
  }
}
