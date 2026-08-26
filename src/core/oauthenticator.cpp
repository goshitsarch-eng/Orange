#include "core/oauthenticator.h"

#include "core/logging.h"
#include "utilities/strutils.h"

#include <gio/gio.h>

#include <cstring>

namespace {

std::string QueryValue(const std::string &query, const std::string &key) {
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

}  // namespace

OAuthenticator::OAuthenticator(NetworkAccessManager *network) : network_(network) {}

OAuthenticator::~OAuthenticator() { StopRedirectServer(); }

std::string OAuthenticator::BuildAuthorizeUrl(const std::string &authorize_url, const std::string &client_id, const std::string &redirect_uri,
                                              const std::string &scope, const std::string &state) {
  gchar *id = g_uri_escape_string(client_id.c_str(), nullptr, TRUE);
  gchar *redir = g_uri_escape_string(redirect_uri.c_str(), nullptr, TRUE);
  gchar *sc = g_uri_escape_string(scope.c_str(), nullptr, TRUE);
  gchar *st = g_uri_escape_string(state.c_str(), nullptr, TRUE);
  std::string url = authorize_url;
  url += (authorize_url.find('?') == std::string::npos ? "?" : "&");
  url += std::string("response_type=code&client_id=") + (id ? id : "") + "&redirect_uri=" + (redir ? redir : "") + "&scope=" + (sc ? sc : "");
  if (state.size()) {
    url += std::string("&state=") + (st ? st : "");
  }
  g_free(id);
  g_free(redir);
  g_free(sc);
  g_free(st);
  return url;
}

bool OAuthenticator::StartRedirectServer() {
  StopRedirectServer();
  service_ = g_socket_service_new();
  GError *error = nullptr;
  GSocketAddress *effective = nullptr;
  GInetAddress *loopback = g_inet_address_new_loopback(G_SOCKET_FAMILY_IPV4);
  GSocketAddress *address = g_inet_socket_address_new(loopback, 0);
  const gboolean added = g_socket_listener_add_address(G_SOCKET_LISTENER(service_), address, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, nullptr,
                                                       &effective, &error);
  g_object_unref(address);
  g_object_unref(loopback);
  if (!added || !effective) {
    if (error) {
      g_error_free(error);
    }
    StopRedirectServer();
    return false;
  }
  const guint16 port = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(effective));
  g_object_unref(effective);
  redirect_uri_ = "http://127.0.0.1:" + std::to_string(port) + "/callback";
  g_signal_connect(service_, "incoming", G_CALLBACK(+[](GSocketService *, GSocketConnection *connection, GObject *, gpointer data) -> gboolean {
                     auto *self = static_cast<OAuthenticator *>(data);
                     GInputStream *input = g_io_stream_get_input_stream(G_IO_STREAM(connection));
                     GOutputStream *output = g_io_stream_get_output_stream(G_IO_STREAM(connection));
                     char buffer[4096] = {};
                     gsize read = 0;
                     g_input_stream_read_all(input, buffer, sizeof(buffer) - 1, &read, nullptr, nullptr);
                     const std::string request(buffer, read);
                     std::string code;
                     const size_t path = request.find("GET ");
                     if (path != std::string::npos) {
                       const size_t space = request.find(' ', path + 4);
                       const std::string target = request.substr(path + 4, space == std::string::npos ? std::string::npos : space - (path + 4));
                       const size_t q = target.find('?');
                       if (q != std::string::npos) {
                         code = QueryValue(target.substr(q + 1), "code");
                       }
                     }
                     const char *body = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
                                        "<html><body><p>You can return to Strawberry.</p></body></html>";
                     g_output_stream_write_all(output, body, std::strlen(body), nullptr, nullptr, nullptr);
                     if (self->callback_) {
                       const auto cb = self->callback_;
                       self->callback_ = {};
                       cb(code, code.empty() ? "No authorization code" : "");
                     }
                     self->StopRedirectServer();
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

void OAuthenticator::AuthorizeInBrowser(const std::string &authorize_url, const std::string &client_id, const std::string &scope, Callback callback) {
  callback_ = std::move(callback);
  if (!StartRedirectServer()) {
    if (callback_) {
      callback_({}, "Could not start redirect server");
      callback_ = {};
    }
    return;
  }
  const std::string url = BuildAuthorizeUrl(authorize_url, client_id, redirect_uri_, scope);
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
                                 Callback callback) {
  if (!network_) {
    callback({}, "No network");
    return;
  }
  gchar *id = g_uri_escape_string(client_id.c_str(), nullptr, TRUE);
  gchar *secret = g_uri_escape_string(client_secret.c_str(), nullptr, TRUE);
  gchar *redir = g_uri_escape_string(redirect_uri_.c_str(), nullptr, TRUE);
  gchar *escaped_code = g_uri_escape_string(code.c_str(), nullptr, TRUE);
  const std::string body = std::string("grant_type=authorization_code&code=") + (escaped_code ? escaped_code : "") +
                           "&redirect_uri=" + (redir ? redir : "") + "&client_id=" + (id ? id : "") + "&client_secret=" + (secret ? secret : "");
  g_free(id);
  g_free(secret);
  g_free(redir);
  g_free(escaped_code);
  network_->Post(token_url, body, [callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({}, response.error.empty() ? "Token exchange failed" : response.error);
      return;
    }
    callback(response.body, {});
  }, "application/x-www-form-urlencoded");
}
