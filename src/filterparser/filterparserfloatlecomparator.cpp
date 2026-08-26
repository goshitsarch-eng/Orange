#include "filterparser/filterparserfloatlecomparator.h"

#include <cstdlib>
#include <string>

FilterParserFloatLeComparator::FilterParserFloatLeComparator(double search_term) : search_term_(search_term) {}

bool FilterParserFloatLeComparator::Matches(const std::string &value) const {
  return std::strtod(value.c_str(), nullptr) <= search_term_;
}

