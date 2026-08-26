#include "filterparser/filterparserintgtcomparator.h"

#include <cstdlib>
#include <string>

FilterParserIntGtComparator::FilterParserIntGtComparator(int search_term) : search_term_(search_term) {}

bool FilterParserIntGtComparator::Matches(const std::string &value) const {
  return static_cast<int>(std::strtol(value.c_str(), nullptr, 10)) > search_term_;
}

