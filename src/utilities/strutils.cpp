#include "utilities/strutils.h"

#include <algorithm>
#include <cctype>

namespace StrUtils {

std::string ToLower(const std::string &value) {
  std::string result = value;
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
  return result;
}

std::string ToUpper(const std::string &value) {
  std::string result = value;
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
  return result;
}

std::string Trim(const std::string &value) {
  const auto not_space = [](unsigned char c) { return !std::isspace(c); };
  auto begin = std::find_if(value.begin(), value.end(), not_space);
  auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
  if (begin >= end) {
    return {};
  }
  return std::string(begin, end);
}

std::vector<std::string> Split(const std::string &value, char delimiter) {
  std::vector<std::string> parts;
  std::string current;
  for (char c : value) {
    if (c == delimiter) {
      parts.push_back(current);
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  parts.push_back(current);
  return parts;
}

std::string Join(const std::vector<std::string> &parts, const std::string &delimiter) {
  std::string result;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i) {
      result += delimiter;
    }
    result += parts[i];
  }
  return result;
}

bool StartsWith(const std::string &value, const std::string &prefix) {
  return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool EndsWith(const std::string &value, const std::string &suffix) {
  return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool ContainsInsensitive(const std::string &haystack, const std::string &needle) {
  return ToLower(haystack).find(ToLower(needle)) != std::string::npos;
}

std::string Replace(const std::string &value, const std::string &from, const std::string &to) {
  if (from.empty()) {
    return value;
  }
  std::string result = value;
  size_t pos = 0;
  while ((pos = result.find(from, pos)) != std::string::npos) {
    result.replace(pos, from.size(), to);
    pos += to.size();
  }
  return result;
}

std::string SqlLikeEscape(const std::string &value) {
  std::string result;
  result.reserve(value.size());
  for (char c : value) {
    if (c == '%' || c == '_' || c == '\\') {
      result.push_back('\\');
    }
    result.push_back(c);
  }
  return result;
}

}  // namespace StrUtils
