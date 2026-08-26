#ifndef STRAWBERRY_NETWORKPROXYFACTORY_H
#define STRAWBERRY_NETWORKPROXYFACTORY_H

#include "constants/networkproxysettings.h"
#include "engine/gstengineproxy.h"

#include <string>

class NetworkProxyFactory {
 public:
  enum class Mode { System, Direct, Manual };

  void ReloadSettings();
  Mode mode() const { return mode_; }
  NetworkProxySettings::ProxyType type() const { return type_; }
  bool use_authentication() const { return use_authentication_; }
  bool engine() const { return engine_; }
  const std::string &hostname() const { return hostname_; }
  int port() const { return port_; }
  std::string ProxyUri() const;
  std::string Scheme() const;
  GstEngineProxy::Options EngineOptions() const;

 private:
  static std::string SystemProxyFromEnv();

  Mode mode_ = Mode::System;
  NetworkProxySettings::ProxyType type_ = NetworkProxySettings::kDefaultType;
  std::string hostname_;
  int port_ = 0;
  bool use_authentication_ = false;
  std::string username_;
  std::string password_;
  bool engine_ = NetworkProxySettings::kDefaultEngine;
};

#endif
