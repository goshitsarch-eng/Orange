#include "filterparser/filterparserfloatnecomparator.h"

#include <cstdlib>
#include <string>

FilterParserFloatNeComparator::FilterParserFloatNeComparator(double search_term) : search_term_(search_term) {}

bool FilterParserFloatNeComparator::Matches(const std::string &value) const {
  return std::strtod(value.c_str(), nullptr) != search_term_;
}

