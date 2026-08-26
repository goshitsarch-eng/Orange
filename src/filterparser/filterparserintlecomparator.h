#ifndef STRAWBERRY_FILTERPARSERINTLECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINTLECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserIntLeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserIntLeComparator(int search_term);
  bool Matches(const std::string &value) const override;

 private:
  int search_term_{};
};

#endif
