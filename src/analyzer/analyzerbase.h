#ifndef STRAWBERRY_ANALYZERBASE_H
#define STRAWBERRY_ANALYZERBASE_H

#include <cairo.h>

#include <string>
#include <vector>

class AnalyzerBase {
 public:
  virtual ~AnalyzerBase() = default;
  virtual std::string name() const = 0;
  virtual void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const = 0;
};

#endif
