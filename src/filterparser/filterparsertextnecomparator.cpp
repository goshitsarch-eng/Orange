#include "filterparser/filterparsertextnecomparator.h"

#include <cstdlib>
#include <string>

FilterParserTextNeComparator::FilterParserTextNeComparator(const std::string & search_term) : search_term_(search_term) {}

bool FilterParserTextNeComparator::Matches(const std::string &value) const {
  return FilterParserTextNeComparator::Normalize(value) != FilterParserTextNeComparator::Normalize(search_term_);
}

std::string FilterParserTextNeComparator::Normalize(const std::string &value) {
  std::string out = value;
  for (char &ch : out) {
    if (ch >= 'A' && ch <= 'Z') {
      ch = static_cast<char>(ch - 'A' + 'a');
    }
  }
  return out;
}

