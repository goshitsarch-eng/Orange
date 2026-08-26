#include "analyzer/rainbowanalyzer.h"

void RainbowAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawBars(cr, width, height, bands, true, false, false);
}
