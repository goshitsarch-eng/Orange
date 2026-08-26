#ifndef STRAWBERRY_SONOGRAMANALYZER_H
#define STRAWBERRY_SONOGRAMANALYZER_H

#include "analyzer/analyzerbase.h"

class SonogramAnalyzer : public AnalyzerBase {
 public:
  std::string name() const override { return "Sonic"; }
  void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const override;
};

#endif
