#include "filterparser/filterparsertextcontainscomparator.h"

#include <cstdlib>
#include <string>

FilterParserTextContainsComparator::FilterParserTextContainsComparator(const std::string & search_term) : search_term_(search_term) {}

bool FilterParserTextContainsComparator::Matches(const std::string &value) const {
  const std::string haystack = FilterParserTextContainsComparator::Normalize(value);
    const std::string needle = FilterParserTextContainsComparator::Normalize(search_term_);
    return haystack.find(needle) != std::string::npos;
}

std::string FilterParserTextContainsComparator::Normalize(const std::string &value) {
  std::string out = value;
  for (char &ch : out) {
    if (ch >= 'A' && ch <= 'Z') {
      ch = static_cast<char>(ch - 'A' + 'a');
    }
  }
  return out;
}

