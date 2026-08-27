#ifndef STRAWBERRY_FILTERPARSERINT64LECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINT64LECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include <cstdint>

class FilterParserInt64LeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserInt64LeComparator(int64_t search_term);
  bool Matches(const std::string &value) const override;

 private:
  int64_t search_term_{};
};

#endif
