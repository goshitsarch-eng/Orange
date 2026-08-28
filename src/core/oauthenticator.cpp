#include "core/oauthenticator.h"

#include "core/httprequestreader.h"
#include "core/logging.h"
#include "utilities/randutils.h"
#include "utilities/strutils.h"

#include <gio/gio.h>
#include <json-glib/json-glib.h>

#include <cstring>
#include <ctime>

OAuthenticator::OAuthenticator(NetworkAccessManager *network) : network_(network) {}

OAuthenticator::~OAuthenticator() {
  *alive_ = false;
  StopRedirectServer();
}

std::string OAuthenticator::BuildAuthorizeUrl(const std::string &authorize_url, const std::string &client_id, const std::string &redirect_uri,
                                              const std::string &scope, const std::string &state, const std::string &code_challenge) {
  gchar *id = g_uri_escape_string(client_id.c_str(), nullptr, TRUE);
  gchar *redir = g_uri_escape_string(redirect_uri.c_str(), nullptr, TRUE);
  gchar *sc = g_uri_escape_string(scope.c_str(), nullptr, TRUE);
  gchar *st = g_uri_escape_string(state.c_str(), nullptr, TRUE);
  gchar *challenge = g_uri_escape_string(code_challenge.c_str(), nullptr, TRUE);
  std::string url = authorize_url;
  url += (authorize_url.find('?') == std::string::npos ? "?" : "&");
  url += std::string("response_type=code&client_id=") + (id ? id : "") + "&redirect_uri=" + (redir ? redir : "") + "&scope=" + (sc ? sc : "");
  if (state.size()) {
    url += std::string("&state=") + (st ? st : "");
  }
  if (code_challenge.size()) {
    url += std::string("&code_challenge_method=S256&code_challenge=") + (challenge ? challenge : "");
  }
  g_free(id);
  g_free(redir);
  g_free(sc);
  g_free(st);
  g_free(challenge);
  return url;
}

std::string OAuthenticator::AuthorizationCodeBody(const std::string &client_id, const std::string &client_secret, const std::string &redirect_uri,
                                                  const std::string &code, const std::string &code_verifier) {
  gchar *id = g_uri_escape_string(client_id.c_str(), nullptr, TRUE);
  gchar *secret = g_uri_escape_string(client_secret.c_str(), nullptr, TRUE);
  gchar *redir = g_uri_escape_string(redirect_uri.c_str(), nullptr, TRUE);
  gchar *escaped_code = g_uri_escape_string(code.c_str(), nullptr, TRUE);
  gchar *verifier = g_uri_escape_string(code_verifier.c_str(), nullptr, TRUE);
  std::string body = std::string("grant_type=authorization_code&code=") + (escaped_code ? escaped_code : "") +
                     "&redirect_uri=" + (redir ? redir : "") + "&client_id=" + (id ? id : "") + "&client_secret=" + (secret ? secret : "");
  if (code_verifier.size()) {
    body += std::string("&code_verifier=") + (verifier ? verifier : "");
  }
  g_free(id);
  g_free(secret);
  g_free(redir);
  g_free(escaped_code);
  g_free(verifier);
  return body;
}

bool OAuthenticator::StartRedirectServer(guint16 preferred_port) {
  StopRedirectServer();
  service_ = g_socket_service_new();
  auto bind_port = [this](guint16 port) -> guint16 {
    GError *error = nullptr;
    GSocketAddress *effective = nullptr;
    GInetAddress *loopback = g_inet_address_new_loopback(G_SOCKET_FAMILY_IPV4);
    GSocketAddress *address = g_inet_socket_address_new(loopback, port);
    const gboolean added =
        g_socket_listener_add_address(G_SOCKET_LISTENER(service_), address, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, nullptr, &effective, &error);
    g_object_unref(address);
    g_object_unref(loopback);
    if (!added || !effective) {
      if (error) {
        g_error_free(error);
      }
      return 0;
    }
    const guint16 bound = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(effective));
    g_object_unref(effective);
    return bound;
  };
  guint16 port = preferred_port > 0 ? bind_port(preferred_port) : 0;
  if (port == 0 && preferred_port > 0) {
    // The provider has this exact redirect URI registered, so falling back to another port would advertise a
    // URI nothing is listening on and hand the authorization code to whatever holds the fixed port instead.
    StopRedirectServer();
    return false;
  }
  if (port == 0) {
    port = bind_port(0);
  }
  if (port == 0) {
    StopRedirectServer();
    return false;
  }
  if (redirect_uri_.empty()) {
    redirect_uri_ = RedirectUriForPort(port);
  }
  g_signal_connect(service_, "incoming", G_CALLBACK(+[](GSocketService *, GSocketConnection *connection, GObject *, gpointer data) -> gboolean {
                     auto *self = static_cast<OAuthenticator *>(data);
                     HttpRequestReader::ReadRequestLine(
                         connection, [self, alive = self->alive_](const std::string &request_line, GSocketConnection *conn) {
                           const std::string target = HttpRequestReader::TargetFromRequestLine(request_line);
                           const std::string code = HttpRequestReader::QueryValue(target, "code");
                           const std::string state = HttpRequestReader::QueryValue(target, "state");
                           const char *body = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
                                              "<html><body><p>You can return to Orange.</p></body></html>";
                           if (conn) {
                             GOutputStream *output = g_io_stream_get_output_stream(G_IO_STREAM(conn));
                             g_output_stream_write_all(output, body, std::strlen(body), nullptr, nullptr, nullptr);
                           }
                           if (!*alive) {
                             return;
                           }
                           std::string error;
                           if (!OAuthState::Matches(self->state_, state)) {
                             // Anything on this machine can hit the loopback redirect; without this check a
                             // local process could feed us an authorization code of its own choosing.
                             error = "Authorization response did not match the request";
                           }
                           else if (code.empty()) {
                             error = "No authorization code";
                           }
                           if (self->callback_) {
                             const auto cb = self->callback_;
                             self->callback_ = {};
                             cb(error.empty() ? code : std::string(), error);
                           }
                           self->StopRedirectServer();
                         });
                     return TRUE;
                   }),
                   this);
  g_socket_service_start(service_);
  return true;
}

void OAuthenticator::StopRedirectServer() {
  if (service_) {
    g_socket_service_stop(service_);
    g_object_unref(service_);
    service_ = nullptr;
  }
}

void OAuthenticator::AuthorizeInBrowser(const std::string &authorize_url, const std::string &client_id, const std::string &scope, Callback callback,
                                        guint16 preferred_port, const std::string &redirect_uri) {
  callback_ = std::move(callback);
  redirect_uri_ = redirect_uri;
  state_ = OAuthState::Generate();
  if (!StartRedirectServer(preferred_port)) {
    if (callback_) {
      callback_({}, "Could not start redirect server");
      callback_ = {};
    }
    return;
  }
  const std::string url = BuildAuthorizeUrl(authorize_url, client_id, redirect_uri_, scope, state_);
  GError *error = nullptr;
  if (!g_app_info_launch_default_for_uri(url.c_str(), nullptr, &error)) {
    if (callback_) {
      callback_({}, error ? error->message : "Could not open browser");
      callback_ = {};
    }
    if (error) {
      g_error_free(error);
    }
    StopRedirectServer();
  }
}

void OAuthenticator::ExchangeCode(const std::string &token_url, const std::string &client_id, const std::string &client_secret, const std::string &code,
                                 Callback callback, const std::string &code_verifier) {
  if (!network_) {
    callback({}, "No network");
    return;
  }
  const std::string body = AuthorizationCodeBody(client_id, client_secret, redirect_uri_, code, code_verifier);
  network_->Post(token_url, body, [callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({}, response.error.empty() ? "Token exchange failed" : response.error);
      return;
    }
    callback(response.body, {});
  }, "application/x-www-form-urlencoded");
}

std::string OAuthenticator::ClientCredentialsBody(const std::string &client_id, const std::string &client_secret) {
  return std::string("grant_type=client_credentials&client_id=") + StrUtils::UriEscape(client_id) +
         "&client_secret=" + StrUtils::UriEscape(client_secret);
}

std::string OAuthenticator::BasicAuthorizationHeader(const std::string &client_id, const std::string &client_secret) {
  const std::string raw = client_id + ":" + client_secret;
  gchar *encoded = g_base64_encode(reinterpret_cast<const guchar *>(raw.data()), raw.size());
  std::string header = std::string("Basic ") + (encoded ? encoded : "");
  g_free(encoded);
  return header;
}

std::string OAuthenticator::RefreshTokenBody(const std::string &refresh_token, const std::string &client_id, const std::string &client_secret) {
  return std::string("grant_type=refresh_token&refresh_token=") + StrUtils::UriEscape(refresh_token) +
         "&client_id=" + StrUtils::UriEscape(client_id) + "&client_secret=" + StrUtils::UriEscape(client_secret);
}

void OAuthenticator::RefreshAccessToken(const std::string &token_url, const std::string &client_id, const std::string &client_secret,
                                        const std::string &refresh_token, Callback callback) {
  if (!network_) {
    callback({}, "No network");
    return;
  }
  network_->Post(token_url, RefreshTokenBody(refresh_token, client_id, client_secret),
                 [callback](const NetworkAccessManager::Response &response) {
                   if (!response.ok()) {
                     callback({}, response.error.empty() ? "Token refresh failed" : response.error);
                     return;
                   }
                   callback(response.body, {});
                 },
                 "application/x-www-form-urlencoded");
}

OAuthenticator::TokenResponse OAuthenticator::ParseTokenResponse(const std::string &json) {
  TokenResponse token;
  if (json.empty()) {
    return token;
  }
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return token;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (root && JSON_NODE_HOLDS_OBJECT(root)) {
    JsonObject *object = json_node_get_object(root);
    auto take_string = [object](const char *key) -> std::string {
      if (!json_object_has_member(object, key) || !JSON_NODE_HOLDS_VALUE(json_object_get_member(object, key)) ||
          json_node_get_value_type(json_object_get_member(object, key)) != G_TYPE_STRING) {
        return {};
      }
      const char *value = json_object_get_string_member(object, key);
      return value ? value : "";
    };
    token.access_token = take_string("access_token");
    token.refresh_token = take_string("refresh_token");
    token.token_type = take_string("token_type");
    if (json_object_has_member(object, "expires_in") && JSON_NODE_HOLDS_VALUE(json_object_get_member(object, "expires_in"))) {
      token.expires_in = static_cast<int>(json_object_get_int_member(object, "expires_in"));
    }
  }
  g_object_unref(parser);
  return token;
}

std::string OAuthenticator::ParseAccessToken(const std::string &json) { return ParseTokenResponse(json).access_token; }

bool OAuthenticator::AccessTokenExpired(gint64 login_time, int expires_in, gint64 now, int skew_seconds) {
  if (login_time <= 0 || expires_in <= 0) {
    return false;
  }
  if (now <= 0) {
    now = static_cast<gint64>(std::time(nullptr));
  }
  return now + skew_seconds >= login_time + expires_in;
}

void OAuthenticator::ClientCredentials(const std::string &token_url, const std::string &client_id, const std::string &client_secret, Callback callback) {
  if (!network_) {
    callback({}, "No network");
    return;
  }
  network_->Post(token_url, ClientCredentialsBody(client_id, client_secret),
                 [callback](const NetworkAccessManager::Response &response) {
                   if (!response.ok()) {
                     callback({}, response.error.empty() ? "Client credentials failed" : response.error);
                     return;
                   }
                   callback(response.body, {});
                 },
                 "application/x-www-form-urlencoded", {{"Authorization", BasicAuthorizationHeader(client_id, client_secret)}});
}
