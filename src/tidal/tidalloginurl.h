#ifndef STRAWBERRY_TIDALLOGINURL_H
#define STRAWBERRY_TIDALLOGINURL_H

#include "core/oauthpkce.h"
#include "core/oauthenticator.h"
#include "utilities/strutils.h"

#include <glib.h>

#include <string>
#include <vector>

namespace TidalLoginUrl {

// Qt TidalService: tidal://login/auth, no local redirect server.
constexpr char kRedirectUri[] = "tidal://login/auth";
constexpr char kAuthorizeUrl[] = "https://login.tidal.com/authorize";
constexpr char kAccessTokenUrl[] = "https://login.tidal.com/oauth2/token";
constexpr char kScope[] = "r_usr w_usr";

inline std::string Scheme(const std::string &url) {
  const size_t sep = url.find("://");
  if (sep == std::string::npos) {
    return {};
  }
  return StrUtils::ToLower(url.substr(0, sep));
}

inline std::string Host(const std::string &url) {
  const size_t sep = url.find("://");
  if (sep == std::string::npos) {
    return {};
  }
  const size_t start = sep + 3;
  const size_t end = url.find_first_of("/?#", start);
  return url.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

// Qt MainWindow::CommandlineOptionsReceived: scheme == "tidal" && host == "login".
inline bool IsLogin(const std::string &url) { return Scheme(url) == "tidal" && Host(url) == "login"; }

inline std::string Find(const std::vector<std::string> &urls) {
  for (const std::string &url : urls) {
    if (IsLogin(url)) {
      return url;
    }
  }
  return {};
}

inline bool ConsumesCommandline(const std::vector<std::string> &urls) { return !Find(urls).empty(); }

inline std::string QueryValue(const std::string &url, const std::string &key) {
  const size_t q = url.find('?');
  if (q == std::string::npos || key.empty()) {
    return {};
  }
  const std::string query = url.substr(q + 1);
  const std::string prefix = key + "=";
  size_t pos = 0;
  while (pos < query.size()) {
    const size_t amp = query.find('&', pos);
    const std::string part = query.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
    if (StrUtils::StartsWith(part, prefix)) {
      gchar *unescaped = g_uri_unescape_string(part.substr(prefix.size()).c_str(), nullptr);
      std::string value = unescaped ? unescaped : part.substr(prefix.size());
      g_free(unescaped);
      return value;
    }
    if (amp == std::string::npos) {
      break;
    }
    pos = amp + 1;
  }
  return {};
}

inline std::string AuthorizationCode(const std::string &url) { return QueryValue(url, "code"); }

inline std::string AuthorizationState(const std::string &url) { return QueryValue(url, "state"); }

inline std::string AuthorizationError(const std::string &url) {
  const std::string description = QueryValue(url, "error_description");
  if (!description.empty()) {
    return description;
  }
  return QueryValue(url, "error");
}

// Qt OAuthenticator::AuthorizationUrlReceived: error, missing code, missing/wrong state.
inline std::string FailureFor(const std::string &url, const std::string &expected_state) {
  const std::string oauth_error = AuthorizationError(url);
  if (!oauth_error.empty()) {
    return oauth_error;
  }
  if (AuthorizationCode(url).empty()) {
    return "No authorization code";
  }
  const std::string state = AuthorizationState(url);
  if (state.empty()) {
    return "Request URL is missing state!";
  }
  if (!OAuthPkce::StateMatches(state, expected_state)) {
    return "Request URL has wrong state " + state + " != " + expected_state;
  }
  return {};
}

inline std::string AuthorizationRequestUrl(const std::string &client_id, const std::string &code_challenge = {}) {
  return OAuthenticator::BuildAuthorizeUrl(kAuthorizeUrl, client_id, kRedirectUri, kScope, code_challenge, code_challenge);
}

}  // namespace TidalLoginUrl

#endif
