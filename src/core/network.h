#ifndef STRAWBERRY_NETWORK_H
#define STRAWBERRY_NETWORK_H

#include "core/signal.h"

#include <gio/gio.h>
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

  int Get(const std::string &url, Callback callback, const std::map<std::string, std::string> &headers = {});
  int Post(const std::string &url, const std::string &body, Callback callback,
           const std::string &content_type = "application/json",
           const std::map<std::string, std::string> &headers = {});
  int Put(const std::string &url, const std::string &body, Callback callback,
          const std::string &content_type = "application/json",
          const std::map<std::string, std::string> &headers = {});
  int Delete(const std::string &url, Callback callback, const std::map<std::string, std::string> &headers = {});
  Response GetSync(const std::string &url, const std::map<std::string, std::string> &headers = {});
  void Cancel(int id);

  void SetProxy(const std::string &proxy_uri);
  void ReloadSettings();
  void ResetConnectionCache();
  SoupSession *session() const { return session_; }

 private:
  struct PendingRequest;

  int Send(SoupMessage *message, Callback callback);
  void Forget(int id);
  void ApplySessionDefaults();
  void WatchNetworkResume();

  SoupSession *session_ = nullptr;
  int next_id_ = 1;
  std::map<int, PendingRequest *> pending_;
  gulong network_changed_id_ = 0;
};

#endif
