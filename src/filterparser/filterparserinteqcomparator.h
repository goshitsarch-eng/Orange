#ifndef STRAWBERRY_FILTERPARSERINTEQCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINTEQCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserIntEqComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserIntEqComparator(int search_term);
  bool Matches(const std::string &value) const override;

 private:
  int search_term_{};
};

#endif
