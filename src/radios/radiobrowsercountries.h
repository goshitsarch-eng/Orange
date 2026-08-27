#ifndef STRAWBERRY_RADIOBROWSERCOUNTRIES_H
#define STRAWBERRY_RADIOBROWSERCOUNTRIES_H

#include "radios/radiobrowsersearchopts.h"

#include <unicode/uloc.h>
#include <unicode/unistr.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace RadioBrowserCountries {

inline bool IsTwoLetter(const char *code) {
  return code && std::strlen(code) == 2 && std::isalpha(static_cast<unsigned char>(code[0])) &&
         std::isalpha(static_cast<unsigned char>(code[1]));
}

inline std::string DisplayName(const char *locale_id) {
  if (!locale_id || !*locale_id) {
    return {};
  }
  UChar display[128];
  UErrorCode status = U_ZERO_ERROR;
  const int32_t len = uloc_getDisplayCountry(locale_id, "en", display, 128, &status);
  if (U_FAILURE(status) || len <= 0) {
    return {};
  }
  std::string name;
  icu::UnicodeString(display, len).toUTF8String(name);
  return name;
}

inline int CompareName(const std::string &a, const std::string &b) {
  const size_t n = std::min(a.size(), b.size());
  for (size_t i = 0; i < n; ++i) {
    const int left = std::tolower(static_cast<unsigned char>(a[i]));
    const int right = std::tolower(static_cast<unsigned char>(b[i]));
    if (left != right) {
      return left < right ? -1 : 1;
    }
  }
  if (a.size() == b.size()) {
    return 0;
  }
  return a.size() < b.size() ? -1 : 1;
}

// Qt RadioSettingsPage::CountryList: unique 2-letter locale territories, sorted by name.
inline std::vector<std::pair<std::string, std::string>> CountryList() {
  std::map<std::string, std::string> by_code;
  const int32_t count = uloc_countAvailable();
  for (int32_t i = 0; i < count; ++i) {
    const char *id = uloc_getAvailable(i);
    char country[8] = {};
    UErrorCode status = U_ZERO_ERROR;
    uloc_getCountry(id, country, sizeof(country), &status);
    if (U_FAILURE(status) || !IsTwoLetter(country) || by_code.count(country) != 0) {
      continue;
    }
    const std::string name = DisplayName(id);
    if (name.empty()) {
      continue;
    }
    by_code.emplace(country, name);
  }
  std::vector<std::pair<std::string, std::string>> countries;
  countries.reserve(by_code.size());
  for (const auto &entry : by_code) {
    countries.emplace_back(entry.first, entry.second);
  }
  std::sort(countries.begin(), countries.end(),
            [](const std::pair<std::string, std::string> &a, const std::pair<std::string, std::string> &b) {
              return CompareName(a.second, b.second) < 0;
            });
  return countries;
}

inline std::vector<std::pair<std::string, std::string>> SettingsChoices() {
  std::vector<std::pair<std::string, std::string>> choices;
  choices.emplace_back("", RadioBrowserSearchOpts::AllCountriesLabel());
  const std::vector<std::pair<std::string, std::string>> countries = CountryList();
  choices.insert(choices.end(), countries.begin(), countries.end());
  return choices;
}

inline bool ContainsCode(const std::vector<std::pair<std::string, std::string>> &countries, const char *code) {
  if (!code) {
    return false;
  }
  for (const auto &entry : countries) {
    if (entry.first == code) {
      return true;
    }
  }
  return false;
}

}  // namespace RadioBrowserCountries

#endif
