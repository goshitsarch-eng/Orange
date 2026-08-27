#ifndef STRAWBERRY_OAUTHPKCE_H
#define STRAWBERRY_OAUTHPKCE_H

#include <glib.h>

#include <string>

namespace OAuthPkce {

// Qt OAuthenticator::Authenticate: Utilities::CryptographicRandomString(44).
constexpr int kVerifierLength = 44;
constexpr char kChallengeMethod[] = "S256";

inline std::string Base64UrlEncode(const unsigned char *data, gsize size) {
  gchar *encoded = g_base64_encode(data, size);
  std::string out = encoded ? encoded : "";
  g_free(encoded);
  for (char &ch : out) {
    if (ch == '+') {
      ch = '-';
    } else if (ch == '/') {
      ch = '_';
    }
  }
  while (!out.empty() && out.back() == '=') {
    out.pop_back();
  }
  return out;
}

// Qt: SHA-256 of the verifier, Base64UrlEncoding, then chop a trailing '='.
inline std::string ChallengeS256(const std::string &verifier) {
  GChecksum *checksum = g_checksum_new(G_CHECKSUM_SHA256);
  g_checksum_update(checksum, reinterpret_cast<const guchar *>(verifier.data()), static_cast<gssize>(verifier.size()));
  gsize length = 32;
  unsigned char digest[32] = {};
  g_checksum_get_digest(checksum, digest, &length);
  g_checksum_free(checksum);
  return Base64UrlEncode(digest, length);
}

inline bool StateMatches(const std::string &state, const std::string &challenge) { return !challenge.empty() && state == challenge; }

}  // namespace OAuthPkce

#endif
