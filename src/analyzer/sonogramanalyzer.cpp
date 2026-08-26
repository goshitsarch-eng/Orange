#include "analyzer/sonogramanalyzer.h"

void SonogramAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawWave(cr, width, height, bands, 0.95, 0.63, 0.35);
}
