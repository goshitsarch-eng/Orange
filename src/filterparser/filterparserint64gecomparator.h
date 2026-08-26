#ifndef STRAWBERRY_FILTERPARSERINT64GECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINT64GECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include <cstdint>

class FilterParserInt64GeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserInt64GeComparator(int64_t search_term);
  bool Matches(const std::string &value) const override;

 private:
  int64_t search_term_{};
};

#endif
