#include "filterparser/filterparserintnecomparator.h"

#include <cstdlib>
#include <string>

FilterParserIntNeComparator::FilterParserIntNeComparator(int search_term) : search_term_(search_term) {}

bool FilterParserIntNeComparator::Matches(const std::string &value) const {
  return static_cast<int>(std::strtol(value.c_str(), nullptr, 10)) != search_term_;
}

