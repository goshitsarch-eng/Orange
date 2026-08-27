#ifndef STRAWBERRY_FILTERPARSERINT64LTCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINT64LTCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include <cstdint>

class FilterParserInt64LtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserInt64LtComparator(int64_t search_term);
  bool Matches(const std::string &value) const override;

 private:
  int64_t search_term_{};
};

#endif
