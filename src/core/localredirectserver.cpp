#include "core/localredirectserver.h"

#include "core/httprequestreader.h"
#include "core/logging.h"

#include <cstring>
#include <string>

LocalRedirectServer::LocalRedirectServer() = default;

LocalRedirectServer::~LocalRedirectServer() {
  *alive_ = false;
  Close();
}

bool LocalRedirectServer::Listen(int port) {
  Close();
  service_ = g_socket_service_new();
  GError *error = nullptr;
  GInetAddress *addr = g_inet_address_new_loopback(G_SOCKET_FAMILY_IPV4);
  GSocketAddress *socket_addr = g_inet_socket_address_new(addr, static_cast<guint16>(port));
  g_object_unref(addr);
  GSocketAddress *effective = nullptr;
  if (!g_socket_listener_add_address(G_SOCKET_LISTENER(service_), socket_addr, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, nullptr,
                                     &effective, &error)) {
    if (error) {
      error_ = error->message;
      LogWarning("LocalRedirectServer: %s", error->message);
      g_error_free(error);
    }
    g_object_unref(socket_addr);
    g_object_unref(service_);
    service_ = nullptr;
    return false;
  }
  g_object_unref(socket_addr);
  if (effective && G_IS_INET_SOCKET_ADDRESS(effective)) {
    port_ = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(effective));
    g_object_unref(effective);
  }
  g_signal_connect(service_, "incoming", G_CALLBACK(+[](GSocketService *svc, GSocketConnection *connection, GObject *source, gpointer data) {
                     return LocalRedirectServer::OnIncoming(svc, connection, source, data);
                   }),
                   this);
  g_socket_service_start(service_);
  return port_ > 0;
}

void LocalRedirectServer::Close() {
  if (service_) {
    g_socket_service_stop(service_);
    g_object_unref(service_);
    service_ = nullptr;
  }
  port_ = 0;
}

std::string LocalRedirectServer::url() const { return "http://127.0.0.1:" + std::to_string(port_) + "/"; }

gboolean LocalRedirectServer::OnIncoming(GSocketService *, GSocketConnection *connection, GObject *, gpointer data) {
  auto *self = static_cast<LocalRedirectServer *>(data);
  HttpRequestReader::ReadRequestLine(
      connection, [self, alive = self->alive_](const std::string &request_line, GSocketConnection *conn) {
        const char *response =
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<html><body>You can close this window.</body></html>";
        if (conn) {
          GOutputStream *output = g_io_stream_get_output_stream(G_IO_STREAM(conn));
          g_output_stream_write_all(output, response, strlen(response), nullptr, nullptr, nullptr);
        }
        if (!*alive) {
          return;
        }
        const std::string target = HttpRequestReader::TargetFromRequestLine(request_line);
        if (!target.empty()) {
          self->redirected_url_ = "http://127.0.0.1:" + std::to_string(self->port_) + target;
        }
        self->Redirected.Emit(self->redirected_url_);
      });
  return TRUE;
}
