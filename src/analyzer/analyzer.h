#ifndef STRAWBERRY_ANALYZER_H
#define STRAWBERRY_ANALYZER_H

#include "analyzer/analyzercontainer.h"
#include "analyzer/fht.h"

#include <cairo.h>

#include <cstdint>
#include <string>
#include <vector>

class Analyzer {
 public:
  Analyzer();
  void SetEngineScope(const std::vector<int16_t> &scope);
  void SetMagnitudes(const std::vector<float> &db);
  const std::vector<float> &bands() const { return bands_; }
  void set_type(const std::string &type);
  const std::string &type() const { return type_; }
  void Draw(cairo_t *cr, int width, int height) const;
  static std::vector<std::string> Types();

 private:
  std::vector<float> bands_ = std::vector<float>(32, 0.0f);
  std::string type_ = "Bar";
  FHT fht_{64};
  AnalyzerContainer container_;
};

#endif
