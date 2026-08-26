#ifndef STRAWBERRY_FILTERPARSERINT64NECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINT64NECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include <cstdint>

class FilterParserInt64NeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserInt64NeComparator(int64_t search_term);
  bool Matches(const std::string &value) const override;

 private:
  int64_t search_term_{};
};

#endif
