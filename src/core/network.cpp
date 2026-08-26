#include "core/network.h"

#include "core/logging.h"
#include "core/networkproxyfactory.h"
#include "core/settings.h"

#include <glib.h>

NetworkAccessManager::NetworkAccessManager() {
  session_ = soup_session_new();
  g_object_set(session_, "user-agent", "Strawberry/1.2.0 (+https://www.strawberrymusicplayer.org)", nullptr);
  NetworkProxyFactory proxy;
  proxy.ReloadSettings();
  const std::string uri = proxy.ProxyUri();
  if (!uri.empty()) {
    SetProxy(uri);
  }
}

NetworkAccessManager::~NetworkAccessManager() {
  if (session_) {
    g_object_unref(session_);
  }
}

void NetworkAccessManager::SetProxy(const std::string &proxy_uri) {
  if (!session_ || proxy_uri.empty()) {
    return;
  }
  GProxyResolver *resolver = g_simple_proxy_resolver_new(proxy_uri.c_str(), nullptr);
  g_object_set(session_, "proxy-resolver", resolver, nullptr);
  g_object_unref(resolver);
}

void NetworkAccessManager::Send(SoupMessage *message, Callback callback) {
  struct PendingRequest {
    Callback callback;
    SoupMessage *message = nullptr;
  };
  auto *pending = new PendingRequest{std::move(callback), message};
  g_object_ref(message);
  soup_session_send_and_read_async(session_, message, G_PRIORITY_DEFAULT, nullptr,
                                   +[](GObject *source, GAsyncResult *result, gpointer user_data) {
                                     auto *pending = static_cast<PendingRequest *>(user_data);
                                     GError *error = nullptr;
                                     GBytes *bytes = soup_session_send_and_read_finish(SOUP_SESSION(source), result, &error);
                                     Response response;
                                     if (pending->message) {
                                       response.status = soup_message_get_status(pending->message);
                                     }
                                     if (error) {
                                       response.error = error->message;
                                       g_error_free(error);
                                     }
                                     if (bytes) {
                                       gsize size = 0;
                                       const gchar *data = static_cast<const gchar *>(g_bytes_get_data(bytes, &size));
                                       response.body.assign(data, size);
                                       g_bytes_unref(bytes);
                                     }
                                     pending->callback(response);
                                     if (pending->message) {
                                       g_object_unref(pending->message);
                                     }
                                     delete pending;
                                   },
                                   pending);
}

void NetworkAccessManager::Get(const std::string &url, Callback callback, const std::map<std::string, std::string> &headers) {
  SoupMessage *message = soup_message_new("GET", url.c_str());
  if (!message) {
    Response response;
    response.error = "Invalid URL";
    callback(response);
    return;
  }
  SoupMessageHeaders *request_headers = soup_message_get_request_headers(message);
  for (const auto &header : headers) {
    soup_message_headers_append(request_headers, header.first.c_str(), header.second.c_str());
  }
  Send(message, std::move(callback));
  g_object_unref(message);
}

void NetworkAccessManager::Post(const std::string &url, const std::string &body, Callback callback, const std::string &content_type,
                                const std::map<std::string, std::string> &headers) {
  SoupMessage *message = soup_message_new("POST", url.c_str());
  if (!message) {
    Response response;
    response.error = "Invalid URL";
    callback(response);
    return;
  }
  GBytes *bytes = g_bytes_new(body.data(), body.size());
  soup_message_set_request_body_from_bytes(message, content_type.c_str(), bytes);
  g_bytes_unref(bytes);
  SoupMessageHeaders *request_headers = soup_message_get_request_headers(message);
  for (const auto &header : headers) {
    soup_message_headers_append(request_headers, header.first.c_str(), header.second.c_str());
  }
  Send(message, std::move(callback));
  g_object_unref(message);
}

void NetworkAccessManager::Put(const std::string &url, const std::string &body, Callback callback, const std::string &content_type,
                               const std::map<std::string, std::string> &headers) {
  SoupMessage *message = soup_message_new("PUT", url.c_str());
  if (!message) {
    Response response;
    response.error = "Invalid URL";
    callback(response);
    return;
  }
  GBytes *bytes = g_bytes_new(body.data(), body.size());
  soup_message_set_request_body_from_bytes(message, content_type.c_str(), bytes);
  g_bytes_unref(bytes);
  SoupMessageHeaders *request_headers = soup_message_get_request_headers(message);
  for (const auto &header : headers) {
    soup_message_headers_append(request_headers, header.first.c_str(), header.second.c_str());
  }
  Send(message, std::move(callback));
  g_object_unref(message);
}

void NetworkAccessManager::Delete(const std::string &url, Callback callback, const std::map<std::string, std::string> &headers) {
  SoupMessage *message = soup_message_new("DELETE", url.c_str());
  if (!message) {
    Response response;
    response.error = "Invalid URL";
    callback(response);
    return;
  }
  SoupMessageHeaders *request_headers = soup_message_get_request_headers(message);
  for (const auto &header : headers) {
    soup_message_headers_append(request_headers, header.first.c_str(), header.second.c_str());
  }
  Send(message, std::move(callback));
  g_object_unref(message);
}

NetworkAccessManager::Response NetworkAccessManager::GetSync(const std::string &url, const std::map<std::string, std::string> &headers) {
  Response response;
  SoupMessage *message = soup_message_new("GET", url.c_str());
  if (!message) {
    response.error = "Invalid URL";
    return response;
  }
  SoupMessageHeaders *request_headers = soup_message_get_request_headers(message);
  for (const auto &header : headers) {
    soup_message_headers_append(request_headers, header.first.c_str(), header.second.c_str());
  }
  GError *error = nullptr;
  GBytes *bytes = soup_session_send_and_read(session_, message, nullptr, &error);
  response.status = soup_message_get_status(message);
  if (error) {
    response.error = error->message;
    g_error_free(error);
  }
  if (bytes) {
    gsize size = 0;
    const gchar *data = static_cast<const gchar *>(g_bytes_get_data(bytes, &size));
    response.body.assign(data, size);
    g_bytes_unref(bytes);
  }
  g_object_unref(message);
  return response;
}
