#ifndef STRAWBERRY_FILTERPARSERTEXTNECOMPARATOR_H
#define STRAWBERRY_FILTERPARSERTEXTNECOMPARATOR_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include <string>

class FilterParserTextNeComparator : public FilterParserSearchTermComparator {
 public:
  explicit FilterParserTextNeComparator(const std::string & search_term);
  bool Matches(const std::string &value) const override;

  static std::string Normalize(const std::string &value);

 private:
  std::string search_term_{};
};

#endif
