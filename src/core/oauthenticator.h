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

  void AuthorizeInBrowser(const std::string &authorize_url, const std::string &client_id, const std::string &scope, Callback callback);
  void ExchangeCode(const std::string &token_url, const std::string &client_id, const std::string &client_secret, const std::string &code, Callback callback);
  void ClientCredentials(const std::string &token_url, const std::string &client_id, const std::string &client_secret, Callback callback);
  std::string redirect_uri() const { return redirect_uri_; }

  static std::string BuildAuthorizeUrl(const std::string &authorize_url, const std::string &client_id, const std::string &redirect_uri,
                                       const std::string &scope, const std::string &state = {});
  static std::string ClientCredentialsBody(const std::string &client_id, const std::string &client_secret);
  static std::string BasicAuthorizationHeader(const std::string &client_id, const std::string &client_secret);
  static std::string ParseAccessToken(const std::string &json);

 private:
  bool StartRedirectServer();
  void StopRedirectServer();

  NetworkAccessManager *network_ = nullptr;
  GSocketService *service_ = nullptr;
  std::string redirect_uri_;
  Callback callback_;
};

#endif
