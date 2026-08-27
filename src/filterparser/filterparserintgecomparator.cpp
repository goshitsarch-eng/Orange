#include "filterparser/filterparserintgecomparator.h"

#include <cstdlib>
#include <string>

FilterParserIntGeComparator::FilterParserIntGeComparator(int search_term) : search_term_(search_term) {}

bool FilterParserIntGeComparator::Matches(const std::string &value) const {
  return static_cast<int>(std::strtol(value.c_str(), nullptr, 10)) >= search_term_;
}

