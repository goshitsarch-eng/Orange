#include "filterparser/filterparserfloatltcomparator.h"

#include <cstdlib>
#include <string>

FilterParserFloatLtComparator::FilterParserFloatLtComparator(double search_term) : search_term_(search_term) {}

bool FilterParserFloatLtComparator::Matches(const std::string &value) const {
  return std::strtod(value.c_str(), nullptr) < search_term_;
}

