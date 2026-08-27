#include "filterparser/filterparseruintgecomparator.h"

#include <cstdlib>
#include <string>

FilterParserUIntGeComparator::FilterParserUIntGeComparator(unsigned search_term) : search_term_(search_term) {}

bool FilterParserUIntGeComparator::Matches(const std::string &value) const {
  return static_cast<unsigned>(std::strtoul(value.c_str(), nullptr, 10)) >= search_term_;
}

