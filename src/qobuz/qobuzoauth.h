#ifndef STRAWBERRY_QOBUZOAUTH_H
#define STRAWBERRY_QOBUZOAUTH_H

#include "settings/streamingsettingslabels.h"
#include "utilities/strutils.h"

#include <glib.h>

#include <string>

namespace QobuzOAuth {

// Qt QobuzService::Authenticate / OAuthRedirectReceived / HandleOAuthCallbackReply
inline constexpr char kSigninUrl[] = "https://www.qobuz.com/signin/oauth";
inline constexpr char kCallbackUrl[] = "https://www.qobuz.com/api.json/0.2/oauth/callback";

inline const char *kMissingCode = "OAuth redirect is missing authorization code.";
inline const char *kMissingToken = "OAuth callback reply is missing token";
inline const char *kMissingJson = "OAuth callback reply from server missing Json data.";
inline const char *kInvalidJson = "OAuth callback reply from server has invalid Json.";
inline const char *kWaitingStatus = "Waiting for browser authentication...";
inline const char *kExchangingStatus = "Exchanging authorization code...";

inline std::string RedirectUrl(int port) { return "http://127.0.0.1:" + std::to_string(port); }

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

inline std::string ExtractCode(const std::string &url) {
  const std::string autorisation = QueryValue(url, "code_autorisation");
  if (!autorisation.empty()) {
    return autorisation;
  }
  return QueryValue(url, "code");
}

inline std::string AuthorizationUrl(const std::string &app_id, int port) {
  return std::string(kSigninUrl) + "?ext_app_id=" + StrUtils::UriEscape(app_id) + "&redirect_url=" +
         StrUtils::UriEscape(RedirectUrl(port));
}

inline std::string CallbackRequestUrl(const std::string &code, const std::string &private_key) {
  return std::string(kCallbackUrl) + "?code=" + StrUtils::UriEscape(code) + "&private_key=" + StrUtils::UriEscape(private_key);
}

inline const char *MissingCredential(const std::string &app_id, const std::string &app_secret, const std::string &private_key) {
  return QobuzSettingsLabels::MissingCredentialMessage(app_id, app_secret, private_key);
}

inline std::string PreferToken(const std::string &token, const std::string &user_auth_token) {
  return token.empty() ? user_auth_token : token;
}

inline std::string ListenFailedMessage(const std::string &error) {
  if (error.empty()) {
    return "Failed to start local server for OAuth redirect.";
  }
  return "Failed to start local server for OAuth redirect: " + error;
}

inline std::string BrowserFailedMessage(const std::string &url) {
  return "Failed to open the web browser. Please open this URL manually: " + url;
}

inline std::string RedirectFailedMessage(const std::string &error) { return "OAuth redirect failed: " + error; }

inline std::string ApiErrorMessage(const std::string &message, int code) { return message + " (" + std::to_string(code) + ")"; }

inline std::string HttpErrorMessage(unsigned status) { return "Received HTTP code " + std::to_string(status); }

}  // namespace QobuzOAuth

#endif  // STRAWBERRY_QOBUZOAUTH_H
