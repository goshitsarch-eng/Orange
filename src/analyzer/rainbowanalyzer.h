#ifndef STRAWBERRY_RAINBOWANALYZER_H
#define STRAWBERRY_RAINBOWANALYZER_H

#include "analyzer/analyzerbase.h"
#include "analyzer/rainbowcanvas.h"

class RainbowAnalyzer : public AnalyzerBase {
 public:
  enum class Style { Classic, Dash, Nyan };

  explicit RainbowAnalyzer(Style style = Style::Classic);
  std::string name() const override { return name_; }
  void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const override;
  void Advance(int width, int height, const std::vector<float> &bands) override;

 private:
  Style style_ = Style::Classic;
  std::string name_ = "Rainbow";
  RainbowCanvas canvas_;
};

#endif
