#ifndef STRAWBERRY_FILTERPARSERUINTEQCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERUINTEQCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserUIntEqComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserUIntEqComparator(unsigned search_term);
  bool Matches(const std::string &value) const override;

 private:
  unsigned search_term_{};
};

#endif
