#ifndef STRAWBERRY_FILTERPARSERUINTNECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERUINTNECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserUIntNeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserUIntNeComparator(unsigned search_term);
  bool Matches(const std::string &value) const override;

 private:
  unsigned search_term_{};
};

#endif
