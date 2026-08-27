#include "filterparser/filterparserint64gtcomparator.h"

#include <cstdlib>
#include <string>

FilterParserInt64GtComparator::FilterParserInt64GtComparator(int64_t search_term) : search_term_(search_term) {}

bool FilterParserInt64GtComparator::Matches(const std::string &value) const {
  return std::strtoll(value.c_str(), nullptr, 10) > search_term_;
}

