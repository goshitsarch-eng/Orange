#ifndef STRAWBERRY_LOCALREDIRECTSERVER_H
#define STRAWBERRY_LOCALREDIRECTSERVER_H

#include "core/signal.h"

#include <gio/gio.h>

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

  Signal<std::string> Redirected;

 private:
  static gboolean OnIncoming(GSocketService *service, GSocketConnection *connection, GObject *source, gpointer data);

  GSocketService *service_ = nullptr;
  int port_ = 0;
  std::string redirected_url_;
};

#endif
