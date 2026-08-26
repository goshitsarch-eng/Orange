#ifndef STRAWBERRY_TURBINEANALYZER_H
#define STRAWBERRY_TURBINEANALYZER_H

#include "analyzer/analyzerbase.h"

class TurbineAnalyzer : public AnalyzerBase {
 public:
  std::string name() const override { return "Turbine"; }
  void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const override;
};

#endif
