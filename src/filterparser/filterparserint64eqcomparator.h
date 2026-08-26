#ifndef STRAWBERRY_FILTERPARSERINT64EQCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINT64EQCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include <cstdint>

class FilterParserInt64EqComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserInt64EqComparator(int64_t search_term);
  bool Matches(const std::string &value) const override;

 private:
  int64_t search_term_{};
};

#endif
