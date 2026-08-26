#ifndef STRAWBERRY_FILTERPARSERINT64GTCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINT64GTCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include <cstdint>

class FilterParserInt64GtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserInt64GtComparator(int64_t search_term);
  bool Matches(const std::string &value) const override;

 private:
  int64_t search_term_{};
};

#endif
