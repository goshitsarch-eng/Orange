#ifndef STRAWBERRY_URLREDACT_H
#define STRAWBERRY_URLREDACT_H

#include <string>

// Replaces the values of query parameters that carry credentials.  Stream URLs for Subsonic and similar
// servers embed the user's password (or a token derived from it) in the query string, so logging one
// verbatim writes the credential to disk in whatever collects the log.
namespace UrlRedact {

inline bool IsSecretParameter(const std::string &name) {
  static const char *kSecret[] = {"p",     "t",         "s",        "password",     "passwd",  "token",     "access_token",
                                  "refresh_token", "api_key", "apikey", "auth",     "api_sig", "sig",       "code",
                                  "client_secret", "session_key", "u",   nullptr};
  for (int i = 0; kSecret[i]; ++i) {
    if (name == kSecret[i]) {
      return true;
    }
  }
  return false;
}

inline std::string Sanitize(const std::string &url) {
  const std::string::size_type question = url.find('?');
  if (question == std::string::npos) {
    return url;
  }
  std::string out = url.substr(0, question + 1);
  std::string::size_type pos = question + 1;
  bool first = true;
  while (pos <= url.size()) {
    const std::string::size_type amp = url.find('&', pos);
    const std::string part = url.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
    const std::string::size_type equals = part.find('=');
    const std::string name = equals == std::string::npos ? part : part.substr(0, equals);
    if (!first) {
      out += "&";
    }
    first = false;
    out += IsSecretParameter(name) ? name + "=<redacted>" : part;
    if (amp == std::string::npos) {
      break;
    }
    pos = amp + 1;
  }
  return out;
}

}  // namespace UrlRedact

#endif
