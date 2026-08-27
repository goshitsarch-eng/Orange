#ifndef STRAWBERRY_FILTERPARSERFLOATGTCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERFLOATGTCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserFloatGtComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatGtComparator(double search_term);
  bool Matches(const std::string &value) const override;

 private:
  double search_term_{};
};

#endif
