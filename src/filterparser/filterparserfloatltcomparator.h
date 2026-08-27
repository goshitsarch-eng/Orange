#ifndef STRAWBERRY_FILTERPARSERFLOATLTCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERFLOATLTCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserFloatLtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatLtComparator(double search_term);
  bool Matches(const std::string &value) const override;

 private:
  double search_term_{};
};

#endif
