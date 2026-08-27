#ifndef STRAWBERRY_FILTERPARSERTEXTEQCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERTEXTEQCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include <string>

class FilterParserTextEqComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserTextEqComparator(const std::string & search_term);
  bool Matches(const std::string &value) const override;

  static std::string Normalize(const std::string &value);

 private:
  std::string search_term_{};
};

#endif
