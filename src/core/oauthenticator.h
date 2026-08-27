#ifndef STRAWBERRY_OAUTHENTICATOR_H
#define STRAWBERRY_OAUTHENTICATOR_H

#include "core/network.h"

#include <gio/gio.h>

#include <functional>
#include <string>

class OAuthenticator {
 public:
  using Callback = std::function<void(const std::string &code_or_token, const std::string &error)>;

  explicit OAuthenticator(NetworkAccessManager *network);
  ~OAuthenticator();

  struct TokenResponse {
    std::string access_token;
    std::string refresh_token;
    std::string token_type;
    int expires_in = 0;
  };

  void AuthorizeInBrowser(const std::string &authorize_url, const std::string &client_id, const std::string &scope, Callback callback,
                          guint16 preferred_port = 0);
  static constexpr guint16 kGeniusRedirectPort = 63111;
  static std::string RedirectUriForPort(guint16 port) {
    if (port == kGeniusRedirectPort) {
      return "http://localhost:63111/";
    }
    return "http://127.0.0.1:" + std::to_string(port) + "/callback";
  }
  void ExchangeCode(const std::string &token_url, const std::string &client_id, const std::string &client_secret, const std::string &code, Callback callback);
  void RefreshAccessToken(const std::string &token_url, const std::string &client_id, const std::string &client_secret,
                          const std::string &refresh_token, Callback callback);
  void ClientCredentials(const std::string &token_url, const std::string &client_id, const std::string &client_secret, Callback callback);
  std::string redirect_uri() const { return redirect_uri_; }

  static std::string BuildAuthorizeUrl(const std::string &authorize_url, const std::string &client_id, const std::string &redirect_uri,
                                       const std::string &scope, const std::string &state = {});
  static std::string ClientCredentialsBody(const std::string &client_id, const std::string &client_secret);
  static std::string RefreshTokenBody(const std::string &refresh_token, const std::string &client_id, const std::string &client_secret);
  static std::string BasicAuthorizationHeader(const std::string &client_id, const std::string &client_secret);
  static std::string ParseAccessToken(const std::string &json);
  static TokenResponse ParseTokenResponse(const std::string &json);
  static bool AccessTokenExpired(gint64 login_time, int expires_in, gint64 now = 0, int skew_seconds = 60);

 private:
  bool StartRedirectServer(guint16 preferred_port = 0);
  void StopRedirectServer();

  NetworkAccessManager *network_ = nullptr;
  GSocketService *service_ = nullptr;
  std::string redirect_uri_;
  Callback callback_;
};

#endif
