#include "filterparser/filterparseruintltcomparator.h"

#include <cstdlib>
#include <string>

FilterParserUIntLtComparator::FilterParserUIntLtComparator(unsigned search_term) : search_term_(search_term) {}

bool FilterParserUIntLtComparator::Matches(const std::string &value) const {
  return static_cast<unsigned>(std::strtoul(value.c_str(), nullptr, 10)) < search_term_;
}

