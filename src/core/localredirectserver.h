#ifndef STRAWBERRY_LOCALREDIRECTSERVER_H
#define STRAWBERRY_LOCALREDIRECTSERVER_H

#include "core/signal.h"

#include <gio/gio.h>

#include <memory>
#include <string>

class LocalRedirectServer {
 public:
  LocalRedirectServer();
  ~LocalRedirectServer();

  bool Listen(int port = 0);
  void Close();
  std::string url() const;
  int port() const { return port_; }
  const std::string &redirected_url() const { return redirected_url_; }
  const std::string &error() const { return error_; }

  Signal<std::string> Redirected;

 private:
  static gboolean OnIncoming(GSocketService *service, GSocketConnection *connection, GObject *source, gpointer data);

  GSocketService *service_ = nullptr;
  int port_ = 0;
  // The request is read asynchronously, so the completion has to check that the server still exists.
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  std::string redirected_url_;
  std::string error_;
};

#endif
