#ifndef STRAWBERRY_ANALYZER_H
#define STRAWBERRY_ANALYZER_H
#include <cstdint>
#include <string>
#include <vector>
class Analyzer {
 public:
  void SetEngineScope(const std::vector<int16_t> &scope);
  const std::vector<float> &bands() const { return bands_; }
  void set_type(const std::string &type) { type_ = type; }
  const std::string &type() const { return type_; }
  static std::vector<std::string> Types();
 private:
  std::vector<float> bands_ = std::vector<float>(32, 0.0f);
  std::string type_ = "Bar";
};
#endif
