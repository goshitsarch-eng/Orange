#ifndef STRAWBERRY_ADDSTREAMURL_H
#define STRAWBERRY_ADDSTREAMURL_H

#include <string>

namespace AddStreamUrl {

inline const char *Title() { return "Add Stream"; }
inline const char *Prompt() { return "Enter the URL of a stream:"; }
inline const char *Add() { return "Add"; }

inline std::string Scheme(const std::string &url) {
  const auto pos = url.find("://");
  if (pos == std::string::npos || pos == 0) {
    return {};
  }
  return url.substr(0, pos);
}

inline std::string Host(const std::string &url) {
  const auto pos = url.find("://");
  if (pos == std::string::npos) {
    return {};
  }
  std::string rest = url.substr(pos + 3);
  if (!rest.empty() && rest.front() == '[') {
    const auto close = rest.find(']');
    if (close == std::string::npos) {
      return {};
    }
    return rest.substr(1, close - 1);
  }
  const auto end = rest.find_first_of("/?#:");
  return end == std::string::npos ? rest : rest.substr(0, end);
}

inline bool IsValid(const std::string &url) { return !Scheme(url).empty() && !Host(url).empty(); }

// Qt AddStreamDialog::TextChanged: OK/Add stays off until the URL has a scheme and host.
inline bool IsComplete(const std::string &url) { return IsValid(url); }

// Qt AddStreamDialog::showEvent focuses and selects the URL unless the show is spontaneous.
inline bool ShouldFocusOnShow() { return true; }

inline bool ShouldSelectAllOnShow() { return true; }

}  // namespace AddStreamUrl

#endif
