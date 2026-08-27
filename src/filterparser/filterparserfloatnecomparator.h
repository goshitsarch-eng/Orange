#ifndef STRAWBERRY_FILTERPARSERFLOATNECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERFLOATNECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserFloatNeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatNeComparator(double search_term);
  bool Matches(const std::string &value) const override;

 private:
  double search_term_{};
};

#endif
