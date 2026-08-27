#include "filterparser/filterparserint64gecomparator.h"

#include <cstdlib>
#include <string>

FilterParserInt64GeComparator::FilterParserInt64GeComparator(int64_t search_term) : search_term_(search_term) {}

bool FilterParserInt64GeComparator::Matches(const std::string &value) const {
  return std::strtoll(value.c_str(), nullptr, 10) >= search_term_;
}

