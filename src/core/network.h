#ifndef STRAWBERRY_NETWORK_H
#define STRAWBERRY_NETWORK_H

#include "core/signal.h"

#include <libsoup/soup.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

class NetworkAccessManager {
 public:
  struct Response {
    unsigned status = 0;
    std::string body;
    std::string error;
    std::map<std::string, std::string> headers;
    bool ok() const { return status >= 200 && status < 300 && error.empty(); }
  };

  using Callback = std::function<void(const Response &)>;

  NetworkAccessManager();
  ~NetworkAccessManager();

  void Get(const std::string &url, Callback callback, const std::map<std::string, std::string> &headers = {});
  void Post(const std::string &url, const std::string &body, Callback callback,
            const std::string &content_type = "application/json",
            const std::map<std::string, std::string> &headers = {});
  Response GetSync(const std::string &url, const std::map<std::string, std::string> &headers = {});

  void SetProxy(const std::string &proxy_uri);
  SoupSession *session() const { return session_; }

 private:
  void Send(SoupMessage *message, Callback callback);

  SoupSession *session_ = nullptr;
};

#endif
