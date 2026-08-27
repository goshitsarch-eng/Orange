#ifndef STRAWBERRY_FILTERPARSERINTLTCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERINTLTCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserIntLtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserIntLtComparator(int search_term);
  bool Matches(const std::string &value) const override;

 private:
  int search_term_{};
};

#endif
