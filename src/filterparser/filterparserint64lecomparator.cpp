#include "filterparser/filterparserint64lecomparator.h"

#include <cstdlib>
#include <string>

FilterParserInt64LeComparator::FilterParserInt64LeComparator(int64_t search_term) : search_term_(search_term) {}

bool FilterParserInt64LeComparator::Matches(const std::string &value) const {
  return std::strtoll(value.c_str(), nullptr, 10) <= search_term_;
}

