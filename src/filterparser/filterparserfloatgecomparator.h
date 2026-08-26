#ifndef STRAWBERRY_FILTERPARSERFLOATGECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERFLOATGECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"

class FilterParserFloatGeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserFloatGeComparator(double search_term);
  bool Matches(const std::string &value) const override;

 private:
  double search_term_{};
};

#endif
