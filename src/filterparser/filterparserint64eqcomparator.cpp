#include "filterparser/filterparserint64eqcomparator.h"

#include <cstdlib>
#include <string>

FilterParserInt64EqComparator::FilterParserInt64EqComparator(int64_t search_term) : search_term_(search_term) {}

bool FilterParserInt64EqComparator::Matches(const std::string &value) const {
  return std::strtoll(value.c_str(), nullptr, 10) == search_term_;
}

