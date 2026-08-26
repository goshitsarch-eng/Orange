#ifndef STRAWBERRY_FILTERPARSERUINTGTCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERUINTGTCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserUIntGtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserUIntGtComparator(unsigned search_term);
  bool Matches(const std::string &value) const override;

 private:
  unsigned search_term_{};
};

#endif
