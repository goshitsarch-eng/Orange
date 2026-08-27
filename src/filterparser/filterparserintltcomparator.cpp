#include "filterparser/filterparserintltcomparator.h"

#include <cstdlib>
#include <string>

FilterParserIntLtComparator::FilterParserIntLtComparator(int search_term) : search_term_(search_term) {}

bool FilterParserIntLtComparator::Matches(const std::string &value) const {
  return static_cast<int>(std::strtol(value.c_str(), nullptr, 10)) < search_term_;
}

