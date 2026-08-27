#include "filterparser/filterparsertexteqcomparator.h"

#include <cstdlib>
#include <string>

FilterParserTextEqComparator::FilterParserTextEqComparator(const std::string & search_term) : search_term_(search_term) {}

bool FilterParserTextEqComparator::Matches(const std::string &value) const {
  return FilterParserTextEqComparator::Normalize(value) == FilterParserTextEqComparator::Normalize(search_term_);
}

std::string FilterParserTextEqComparator::Normalize(const std::string &value) {
  std::string out = value;
  for (char &ch : out) {
    if (ch >= 'A' && ch <= 'Z') {
      ch = static_cast<char>(ch - 'A' + 'a');
    }
  }
  return out;
}

