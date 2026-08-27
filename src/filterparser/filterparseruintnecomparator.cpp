#include "filterparser/filterparseruintnecomparator.h"

#include <cstdlib>
#include <string>

FilterParserUIntNeComparator::FilterParserUIntNeComparator(unsigned search_term) : search_term_(search_term) {}

bool FilterParserUIntNeComparator::Matches(const std::string &value) const {
  return static_cast<unsigned>(std::strtoul(value.c_str(), nullptr, 10)) != search_term_;
}

