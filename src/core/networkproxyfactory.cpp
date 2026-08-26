#include "core/networkproxyfactory.h"

#include "core/settings.h"

void NetworkProxyFactory::ReloadSettings() {
  Settings settings;
  settings.BeginGroup("NetworkProxy");
  const std::string type = settings.Value("type");
  const int mode = settings.IntValue("mode", -1);
  if (mode == 0 || type == "none" || type == "direct") {
    mode_ = Mode::Direct;
  } else if (mode == 2 || type == "manual" || type == "http" || type == "socks" || type == "3" || type == "1") {
    mode_ = Mode::Manual;
  } else {
    mode_ = Mode::System;
  }
  hostname_ = settings.Value("hostname");
  port_ = settings.IntValue("port", 0);
}

std::string NetworkProxyFactory::ProxyUri() const {
  if (mode_ != Mode::Manual || hostname_.empty() || port_ <= 0) {
    return {};
  }
  return "http://" + hostname_ + ":" + std::to_string(port_);
}
