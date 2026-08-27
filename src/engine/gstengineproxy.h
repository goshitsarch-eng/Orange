#ifndef STRAWBERRY_GSTENGINEPROXY_H
#define STRAWBERRY_GSTENGINEPROXY_H

#include "constants/networkproxysettings.h"

#include <string>

namespace GstEngineProxy {

struct Options {
  std::string address;
  bool authentication = false;
  std::string user;
  std::string pass;
};

inline bool ShouldApply(NetworkProxySettings::Mode mode, NetworkProxySettings::ProxyType type, bool engine) {
  return mode == NetworkProxySettings::Mode::Manual && type == NetworkProxySettings::ProxyType::HttpProxy && engine;
}

inline std::string Address(const std::string &hostname, int port) {
  if (hostname.empty() || port <= 0) {
    return {};
  }
  return hostname + ":" + std::to_string(port);
}

inline Options FromSettings(NetworkProxySettings::Mode mode, NetworkProxySettings::ProxyType type, bool engine, const std::string &hostname,
                            int port, bool authentication, const std::string &user, const std::string &pass) {
  Options options;
  if (!ShouldApply(mode, type, engine)) {
    return options;
  }
  options.address = Address(hostname, port);
  if (options.address.empty()) {
    return {};
  }
  options.authentication = authentication;
  options.user = user;
  options.pass = pass;
  return options;
}

}  // namespace GstEngineProxy

#endif  // STRAWBERRY_GSTENGINEPROXY_H
