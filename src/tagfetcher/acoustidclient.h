#ifndef STRAWBERRY_ACOUSTIDCLIENT_H
#define STRAWBERRY_ACOUSTIDCLIENT_H

#include "core/network.h"
#include "core/signal.h"

#include <string>
#include <vector>

class AcoustidClient {
 public:
  explicit AcoustidClient(NetworkAccessManager *network);

  void SetTimeout(int msec) { timeout_msec_ = msec; }
  void Start(int id, const std::string &fingerprint, int duration_msec);
  void Cancel(int id);
  void CancelAll();

  static std::vector<std::string> ParseMbids(const std::string &json);

  Signal<int, std::vector<std::string>, std::string> Finished;

 private:
  NetworkAccessManager *network_ = nullptr;
  int timeout_msec_ = 5000;
};

#endif
