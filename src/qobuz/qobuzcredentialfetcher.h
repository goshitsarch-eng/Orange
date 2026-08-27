#ifndef STRAWBERRY_QOBUZCREDENTIALFETCHER_H
#define STRAWBERRY_QOBUZCREDENTIALFETCHER_H

#include "core/network.h"

#include <functional>
#include <string>

class QobuzCredentialFetcher {
 public:
  using Callback = std::function<void(const std::string &app_id, const std::string &app_secret, const std::string &private_key,
                                      const std::string &error)>;
  static void Fetch(NetworkAccessManager *network, Callback callback);
};

#endif
