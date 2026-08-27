#ifndef STRAWBERRY_FILTERPARSERINTNECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINTNECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserIntNeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserIntNeComparator(int search_term);
  bool Matches(const std::string &value) const override;

 private:
  int search_term_{};
};

#endif
