#include "filterparser/filterparserfloatgtcomparator.h"

#include <cstdlib>
#include <string>

FilterParserFloatGtComparator::FilterParserFloatGtComparator(double search_term) : search_term_(search_term) {}

bool FilterParserFloatGtComparator::Matches(const std::string &value) const {
  return std::strtod(value.c_str(), nullptr) > search_term_;
}

