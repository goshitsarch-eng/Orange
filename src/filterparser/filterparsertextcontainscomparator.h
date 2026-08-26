#ifndef STRAWBERRY_FILTERPARSERTEXTCONTAINSCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERTEXTCONTAINSCOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include <string>

class FilterParserTextContainsComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserTextContainsComparator(const std::string & search_term);
  bool Matches(const std::string &value) const override;

  static std::string Normalize(const std::string &value);

 private:
  std::string search_term_{};
};

#endif
