#ifndef STRAWBERRY_LISTENBRAINZOAUTH_H
#define STRAWBERRY_LISTENBRAINZOAUTH_H

#include "core/oauthenticator.h"
#include "utilities/strutils.h"

#include <glib.h>

#include <string>

namespace ListenBrainzOAuth {

// Qt ListenBrainzScrobbler MusicBrainz OAuth (src/scrobbler/listenbrainzscrobbler.cpp).
inline constexpr char kAuthorizeUrl[] = "https://musicbrainz.org/oauth2/authorize";
inline constexpr char kTokenUrl[] = "https://musicbrainz.org/oauth2/token";
inline constexpr char kScope[] = "profile;email;tag;rating;collection;submit_isrc;submit_barcode";
inline constexpr char kClientIdB64[] = "b2VBVU53cVNRZXIwZXIwOUZpcWkwUQ==";
inline constexpr char kClientSecretB64[] = "Uk9GZ2hrZVEzRjNvUHlFaHFpeVdQQQ==";

inline std::string DecodeB64(const char *b64) {
  gsize len = 0;
  gchar *raw = reinterpret_cast<gchar *>(g_base64_decode(b64, &len));
  std::string value(raw ? raw : "", raw ? len : 0);
  g_free(raw);
  return value;
}

inline std::string ClientId() { return DecodeB64(kClientIdB64); }

inline std::string ClientSecret() { return DecodeB64(kClientSecretB64); }

// Qt OAuthenticator: http://localhost plus the random listen port.
inline std::string RedirectUri(int port) { return "http://localhost:" + std::to_string(port) + "/"; }

inline std::string AuthorizationUrl(const std::string &redirect_uri, const std::string &code_challenge = {}) {
  return OAuthenticator::BuildAuthorizeUrl(kAuthorizeUrl, ClientId(), redirect_uri, kScope, code_challenge, code_challenge);
}

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

inline std::string ExtractCode(const std::string &url) { return QueryValue(url, "code"); }

inline bool ShouldStartAuthorization(NetworkAccessManager *network) { return network != nullptr; }

inline bool LoginWidgetSignedIn(bool has_user_token, bool has_oauth) { return has_user_token || has_oauth; }

}  // namespace ListenBrainzOAuth

#endif
