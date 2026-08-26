#ifndef STRAWBERRY_SONOGRAMANALYZER_H
#define STRAWBERRY_SONOGRAMANALYZER_H

#include "analyzer/analyzerbase.h"
#include "analyzer/sonogramcanvas.h"

class SonogramAnalyzer : public AnalyzerBase {
 public:
  std::string name() const override { return "Sonic"; }
  void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const override;
  void Advance(int width, int height, const std::vector<float> &bands) override;

 private:
  mutable SonogramCanvas::Buffer canvas_;
};

#endif
