#ifndef STRAWBERRY_FILTERPARSERFLOATLECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERFLOATLECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserFloatLeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatLeComparator(double search_term);
  bool Matches(const std::string &value) const override;

 private:
  double search_term_{};
};

#endif
