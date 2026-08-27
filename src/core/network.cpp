#include "core/network.h"

#include "core/logging.h"
#include "core/networkproxyfactory.h"
#include "core/networkresume.h"
#include "core/settings.h"

#include <glib.h>

namespace {

struct PendingRequest {
  NetworkAccessManager::Callback callback;
  SoupMessage *message = nullptr;
  GCancellable *cancellable = nullptr;
  int id = 0;
  NetworkAccessManager *self = nullptr;
};

void OnNetworkChanged(GNetworkMonitor *, gboolean available, gpointer data) {
  auto *self = static_cast<NetworkAccessManager *>(data);
  if (self && NetworkResume::ShouldClearConnectionCache(available != FALSE)) {
    self->ResetConnectionCache();
  }
}

}  // namespace

NetworkAccessManager::NetworkAccessManager() {
  session_ = soup_session_new();
  ApplySessionDefaults();
  ReloadSettings();
  WatchNetworkResume();
}

NetworkAccessManager::~NetworkAccessManager() {
  if (network_changed_id_) {
    g_signal_handler_disconnect(g_network_monitor_get_default(), network_changed_id_);
    network_changed_id_ = 0;
  }
  if (session_) {
    soup_session_abort(session_);
    g_object_unref(session_);
  }
}

void NetworkAccessManager::ApplySessionDefaults() {
  if (!session_) {
    return;
  }
  g_object_set(session_, "user-agent", NetworkResume::kUserAgent, nullptr);
}

void NetworkAccessManager::WatchNetworkResume() {
  GNetworkMonitor *monitor = g_network_monitor_get_default();
  if (!monitor) {
    return;
  }
  network_changed_id_ = g_signal_connect(monitor, NetworkResume::kNetworkChangedSignal, G_CALLBACK(OnNetworkChanged), this);
}

void NetworkAccessManager::ResetConnectionCache() {
  if (!session_) {
    return;
  }
  SoupSession *old = session_;
  session_ = soup_session_new();
  ApplySessionDefaults();
  ReloadSettings();
  g_object_unref(old);
}

void NetworkAccessManager::SetProxy(const std::string &proxy_uri) {
  if (!session_) {
    return;
  }
  if (proxy_uri.empty()) {
    g_object_set(session_, "proxy-resolver", static_cast<GProxyResolver *>(nullptr), nullptr);
    return;
  }
  GProxyResolver *resolver = g_simple_proxy_resolver_new(proxy_uri.c_str(), nullptr);
  g_object_set(session_, "proxy-resolver", resolver, nullptr);
  g_object_unref(resolver);
}

void NetworkAccessManager::ReloadSettings() {
  NetworkProxyFactory proxy;
  proxy.ReloadSettings();
  if (proxy.mode() == NetworkProxyFactory::Mode::Direct) {
    SetProxy("direct://");
    return;
  }
  SetProxy(proxy.ProxyUri());
}

void NetworkAccessManager::Forget(int id) { cancellables_.erase(id); }

void NetworkAccessManager::Cancel(int id) {
  auto it = cancellables_.find(id);
  if (it == cancellables_.end() || !it->second) {
    return;
  }
  g_cancellable_cancel(it->second);
}

int NetworkAccessManager::Send(SoupMessage *message, Callback callback) {
  const int id = next_id_++;
  GCancellable *cancellable = g_cancellable_new();
  auto *pending = new PendingRequest{std::move(callback), message, cancellable, id, this};
  g_object_ref(message);
  cancellables_[id] = cancellable;
  soup_session_send_and_read_async(session_, message, G_PRIORITY_DEFAULT, cancellable,
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
                                     if (pending->self) {
                                       pending->self->Forget(pending->id);
                                     }
                                     pending->callback(response);
                                     if (pending->message) {
                                       g_object_unref(pending->message);
                                     }
                                     if (pending->cancellable) {
                                       g_object_unref(pending->cancellable);
                                     }
                                     delete pending;
                                   },
                                   pending);
  return id;
}

int NetworkAccessManager::Get(const std::string &url, Callback callback, const std::map<std::string, std::string> &headers) {
  SoupMessage *message = soup_message_new("GET", url.c_str());
  if (!message) {
    Response response;
    response.error = "Invalid URL";
    callback(response);
    return 0;
  }
  SoupMessageHeaders *request_headers = soup_message_get_request_headers(message);
  for (const auto &header : headers) {
    soup_message_headers_append(request_headers, header.first.c_str(), header.second.c_str());
  }
  const int id = Send(message, std::move(callback));
  g_object_unref(message);
  return id;
}

int NetworkAccessManager::Post(const std::string &url, const std::string &body, Callback callback, const std::string &content_type,
                               const std::map<std::string, std::string> &headers) {
  SoupMessage *message = soup_message_new("POST", url.c_str());
  if (!message) {
    Response response;
    response.error = "Invalid URL";
    callback(response);
    return 0;
  }
  GBytes *bytes = g_bytes_new(body.data(), body.size());
  soup_message_set_request_body_from_bytes(message, content_type.c_str(), bytes);
  g_bytes_unref(bytes);
  SoupMessageHeaders *request_headers = soup_message_get_request_headers(message);
  for (const auto &header : headers) {
    soup_message_headers_append(request_headers, header.first.c_str(), header.second.c_str());
  }
  const int id = Send(message, std::move(callback));
  g_object_unref(message);
  return id;
}

int NetworkAccessManager::Put(const std::string &url, const std::string &body, Callback callback, const std::string &content_type,
                              const std::map<std::string, std::string> &headers) {
  SoupMessage *message = soup_message_new("PUT", url.c_str());
  if (!message) {
    Response response;
    response.error = "Invalid URL";
    callback(response);
    return 0;
  }
  GBytes *bytes = g_bytes_new(body.data(), body.size());
  soup_message_set_request_body_from_bytes(message, content_type.c_str(), bytes);
  g_bytes_unref(bytes);
  SoupMessageHeaders *request_headers = soup_message_get_request_headers(message);
  for (const auto &header : headers) {
    soup_message_headers_append(request_headers, header.first.c_str(), header.second.c_str());
  }
  const int id = Send(message, std::move(callback));
  g_object_unref(message);
  return id;
}

int NetworkAccessManager::Delete(const std::string &url, Callback callback, const std::map<std::string, std::string> &headers) {
  SoupMessage *message = soup_message_new("DELETE", url.c_str());
  if (!message) {
    Response response;
    response.error = "Invalid URL";
    callback(response);
    return 0;
  }
  SoupMessageHeaders *request_headers = soup_message_get_request_headers(message);
  for (const auto &header : headers) {
    soup_message_headers_append(request_headers, header.first.c_str(), header.second.c_str());
  }
  const int id = Send(message, std::move(callback));
  g_object_unref(message);
  return id;
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
