#ifndef STRAWBERRY_FILTERPARSERUINTLTCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERUINTLTCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserUIntLtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserUIntLtComparator(unsigned search_term);
  bool Matches(const std::string &value) const override;

 private:
  unsigned search_term_{};
};

#endif
