#include "filterparser/filterparseruinteqcomparator.h"

#include <cstdlib>
#include <string>

FilterParserUIntEqComparator::FilterParserUIntEqComparator(unsigned search_term) : search_term_(search_term) {}

bool FilterParserUIntEqComparator::Matches(const std::string &value) const {
  return static_cast<unsigned>(std::strtoul(value.c_str(), nullptr, 10)) == search_term_;
}

