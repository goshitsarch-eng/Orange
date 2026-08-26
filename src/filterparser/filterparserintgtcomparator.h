#ifndef STRAWBERRY_FILTERPARSERINTGTCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINTGTCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserIntGtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserIntGtComparator(int search_term);
  bool Matches(const std::string &value) const override;

 private:
  int search_term_{};
};

#endif
