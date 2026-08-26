#ifndef STRAWBERRY_RAINBOWANALYZER_H
#define STRAWBERRY_RAINBOWANALYZER_H

#include "analyzer/analyzerbase.h"

class RainbowAnalyzer : public AnalyzerBase {
 public:
  enum class Style { Classic, Dash, Nyan };

  explicit RainbowAnalyzer(Style style = Style::Classic);
  std::string name() const override { return name_; }
  void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const override;

 private:
  Style style_ = Style::Classic;
  std::string name_ = "Rainbow";
};

#endif
