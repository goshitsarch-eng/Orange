#ifndef STRAWBERRY_FILTERPARSERUINTGECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERUINTGECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserUIntGeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserUIntGeComparator(unsigned search_term);
  bool Matches(const std::string &value) const override;

 private:
  unsigned search_term_{};
};

#endif
