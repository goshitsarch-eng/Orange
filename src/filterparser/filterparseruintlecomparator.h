#ifndef STRAWBERRY_FILTERPARSERUINTLECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERUINTLECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserUIntLeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserUIntLeComparator(unsigned search_term);
  bool Matches(const std::string &value) const override;

 private:
  unsigned search_term_{};
};

#endif
