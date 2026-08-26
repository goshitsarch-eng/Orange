#ifndef STRAWBERRY_STREAMINGAUTH_H
#define STRAWBERRY_STREAMINGAUTH_H

#include "core/oauthenticator.h"
#include "core/settings.h"

#include <string>
#include <vector>

class StreamingAuth {
 public:
  enum class Action { Proceed, Refresh };

  static Action EnsureAction(bool expired, bool has_refresh_token) {
    return expired && has_refresh_token ? Action::Refresh : Action::Proceed;
  }

  static Action EnsureAction(gint64 login_time, int expires_in, const std::string &refresh_token, gint64 now = 0) {
    return EnsureAction(OAuthenticator::AccessTokenExpired(login_time, expires_in, now), !refresh_token.empty());
  }

  static void ClearKeys(const std::string &group, const std::vector<const char *> &keys) {
    Settings settings;
    settings.BeginGroup(group);
    for (const char *key : keys) {
      if (key) {
        settings.Remove(key);
      }
    }
    settings.Sync();
  }
};

#endif
