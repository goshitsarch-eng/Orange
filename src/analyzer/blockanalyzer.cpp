#include "analyzer/blockanalyzer.h"

void BlockAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawBars(cr, width, height, bands, false, false, true);
}
