#include "analyzer/waverubberanalyzer.h"

void WaveRubberAnalyzer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const {
  DrawWave(cr, width, height, bands, 0.23, 0.63, 0.95);
}
