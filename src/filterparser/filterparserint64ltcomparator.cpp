#include "filterparser/filterparserint64ltcomparator.h"

#include <cstdlib>
#include <string>

FilterParserInt64LtComparator::FilterParserInt64LtComparator(int64_t search_term) : search_term_(search_term) {}

bool FilterParserInt64LtComparator::Matches(const std::string &value) const {
  return std::strtoll(value.c_str(), nullptr, 10) < search_term_;
}

