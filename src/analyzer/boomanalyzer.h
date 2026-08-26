#ifndef STRAWBERRY_BOOMANALYZER_H
#define STRAWBERRY_BOOMANALYZER_H

#include "analyzer/analyzerbase.h"

class BoomAnalyzer : public AnalyzerBase {
 public:
  std::string name() const override { return "Bar"; }
  void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const override;
};

#endif
