#ifndef STRAWBERRY_WAVERUBBERANALYZER_H
#define STRAWBERRY_WAVERUBBERANALYZER_H

#include "analyzer/analyzerbase.h"

class WaveRubberAnalyzer : public AnalyzerBase {
 public:
  std::string name() const override { return "Wave"; }
  void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const override;
};

#endif
