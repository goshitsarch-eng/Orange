#include "filterparser/filterparseruintgtcomparator.h"

#include <cstdlib>
#include <string>

FilterParserUIntGtComparator::FilterParserUIntGtComparator(unsigned search_term) : search_term_(search_term) {}

bool FilterParserUIntGtComparator::Matches(const std::string &value) const {
  return static_cast<unsigned>(std::strtoul(value.c_str(), nullptr, 10)) > search_term_;
}

