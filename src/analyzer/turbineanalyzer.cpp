#include "analyzer/turbineanalyzer.h"

void TurbineAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawBars(cr, width, height, bands, false, true, false);
}
