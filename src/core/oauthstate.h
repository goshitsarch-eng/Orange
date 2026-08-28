#ifndef STRAWBERRY_OAUTHSTATE_H
#define STRAWBERRY_OAUTHSTATE_H

#include "utilities/randutils.h"

#include <string>

// The OAuth "state" parameter ties an authorization response back to the request that started it.  The
// redirect lands on a loopback port that any process on the machine can reach, so without it a local
// attacker can deliver an authorization code of their own and have the account linked to theirs.
namespace OAuthState {

inline constexpr int kLength = 32;

inline std::string Generate() { return RandUtils::CryptographicRandomString(kLength); }

// A response matches when it carries back exactly the state that was sent.  An empty expected state means no
// state was sent, which is only the case for flows that never started one.
inline bool Matches(const std::string &expected, const std::string &received) {
  if (expected.empty()) {
    return true;
  }
  if (expected.size() != received.size()) {
    return false;
  }
  // Constant-time compare, so a wrong guess cannot be refined a character at a time.
  unsigned char diff = 0;
  for (std::string::size_type i = 0; i < expected.size(); ++i) {
    diff |= static_cast<unsigned char>(expected[i]) ^ static_cast<unsigned char>(received[i]);
  }
  return diff == 0;
}

}  // namespace OAuthState

#endif
