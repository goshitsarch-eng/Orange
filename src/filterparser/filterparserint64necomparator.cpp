#include "filterparser/filterparserint64necomparator.h"

#include <cstdlib>
#include <string>

FilterParserInt64NeComparator::FilterParserInt64NeComparator(int64_t search_term) : search_term_(search_term) {}

bool FilterParserInt64NeComparator::Matches(const std::string &value) const {
  return std::strtoll(value.c_str(), nullptr, 10) != search_term_;
}

