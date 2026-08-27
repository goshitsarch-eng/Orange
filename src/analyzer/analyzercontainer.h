#ifndef STRAWBERRY_ANALYZERCONTAINER_H
#define STRAWBERRY_ANALYZERCONTAINER_H

#include "analyzer/analyzerbase.h"

#include <cairo.h>

#include <memory>
#include <string>
#include <vector>

class AnalyzerContainer {
 public:
  AnalyzerContainer();
  void set_type(const std::string &type);
  std::string type() const;
  void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const;
  void Advance(int width, int height, const std::vector<float> &bands);
  static std::unique_ptr<AnalyzerBase> Create(const std::string &type);

 private:
  std::unique_ptr<AnalyzerBase> current_;
};

#endif
