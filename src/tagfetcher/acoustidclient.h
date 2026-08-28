#ifndef STRAWBERRY_ACOUSTIDCLIENT_H
#define STRAWBERRY_ACOUSTIDCLIENT_H

#include "core/network.h"
#include "core/networktimeouts.h"
#include "core/signal.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class AcoustidClient {
 public:
  explicit AcoustidClient(NetworkAccessManager *network);
  ~AcoustidClient();

  void SetTimeout(int msec) {
    timeout_msec_ = msec;
    timeouts_.SetTimeout(msec);
  }
  void Start(int id, const std::string &fingerprint, int duration_msec);
  void Cancel(int id);
  void CancelAll();

  static std::vector<std::string> ParseMbids(const std::string &json);

  Signal<int, std::vector<std::string>, std::string> Finished;

 private:
  NetworkAccessManager *network_ = nullptr;
  NetworkTimeouts timeouts_;
  int timeout_msec_ = 5000;
  std::map<int, int> requests_;
  // Network replies arrive long after this client can be destroyed, so callbacks hold a copy of this flag
  // instead of trusting the pointer they captured.
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

#endif
