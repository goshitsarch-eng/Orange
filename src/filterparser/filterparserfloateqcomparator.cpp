#include "filterparser/filterparserfloateqcomparator.h"

#include <cstdlib>
#include <string>

FilterParserFloatEqComparator::FilterParserFloatEqComparator(double search_term) : search_term_(search_term) {}

bool FilterParserFloatEqComparator::Matches(const std::string &value) const {
  return std::strtod(value.c_str(), nullptr) == search_term_;
}

