#ifndef STRAWBERRY_FILTERPARSERINTGECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINTGECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserIntGeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserIntGeComparator(int search_term);
  bool Matches(const std::string &value) const override;

 private:
  int search_term_{};
};

#endif
