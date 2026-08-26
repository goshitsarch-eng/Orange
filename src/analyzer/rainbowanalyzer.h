#ifndef STRAWBERRY_RAINBOWANALYZER_H
#define STRAWBERRY_RAINBOWANALYZER_H

#include "analyzer/analyzerbase.h"

class RainbowAnalyzer : public AnalyzerBase {
 public:
  std::string name() const override { return "Rainbow"; }
  void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const override;
};

#endif
