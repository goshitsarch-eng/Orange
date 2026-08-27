#include "filterparser/filterparserfloatgecomparator.h"

#include <cstdlib>
#include <string>

FilterParserFloatGeComparator::FilterParserFloatGeComparator(double search_term) : search_term_(search_term) {}

bool FilterParserFloatGeComparator::Matches(const std::string &value) const {
  return std::strtod(value.c_str(), nullptr) >= search_term_;
}

