#ifndef STRAWBERRY_FILTERPARSERSEARCHTERMCOMPARATOR_H
#define STRAWBERRY_FILTERPARSERSEARCHTERMCOMPARATOR_H

#include <string>

class FilterParserSearchTermComparator {
 public:
  virtual ~FilterParserSearchTermComparator() = default;
  virtual bool Matches(const std::string &value) const = 0;
};

#endif
