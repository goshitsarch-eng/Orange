#ifndef STRAWBERRY_BLOCKANALYZER_H
#define STRAWBERRY_BLOCKANALYZER_H

#include "analyzer/analyzerbase.h"
#include "analyzer/blockanalyzerstate.h"

class BlockAnalyzer : public AnalyzerBase {
 public:
  std::string name() const override { return "Block"; }
  void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const override;
  void Advance(int width, int height, const std::vector<float> &bands) override;

 private:
  BlockAnalyzerState state_;
};

#endif
