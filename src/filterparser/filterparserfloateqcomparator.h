#ifndef STRAWBERRY_FILTERPARSERFLOATEQCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERFLOATEQCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserFloatEqComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatEqComparator(double search_term);
  bool Matches(const std::string &value) const override;

 private:
  double search_term_{};
};

#endif
