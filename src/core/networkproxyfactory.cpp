#include "core/networkproxyfactory.h"

#include "core/settings.h"

#include <cstdlib>

void NetworkProxyFactory::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(NetworkProxySettings::kSettingsGroup);
  const std::string type = settings.Value(NetworkProxySettings::kType);
  const int mode = settings.IntValue(NetworkProxySettings::kMode, -1);
  if (mode == static_cast<int>(NetworkProxySettings::Mode::Direct) || type == "none" || type == "direct") {
    mode_ = Mode::Direct;
  } else if (mode == static_cast<int>(NetworkProxySettings::Mode::Manual) || type == "manual" || type == "http" || type == "socks" ||
             type == "3" || type == "1") {
    mode_ = Mode::Manual;
  } else {
    mode_ = Mode::System;
  }
  if (type == "socks" || type == "1") {
    type_ = NetworkProxySettings::ProxyType::Socks5Proxy;
  } else if (type == "http" || type == "3" || type == "manual") {
    type_ = NetworkProxySettings::ProxyType::HttpProxy;
  } else if (settings.Contains(NetworkProxySettings::kType)) {
    type_ = static_cast<NetworkProxySettings::ProxyType>(settings.IntValue(NetworkProxySettings::kType, static_cast<int>(NetworkProxySettings::kDefaultType)));
  } else {
    type_ = NetworkProxySettings::kDefaultType;
  }
  hostname_ = settings.Value(NetworkProxySettings::kHostname);
  port_ = settings.IntValue(NetworkProxySettings::kPort, static_cast<int>(NetworkProxySettings::kDefaultPort));
  use_authentication_ = settings.BoolValue(NetworkProxySettings::kUseAuthentication, NetworkProxySettings::kDefaultUseAuthentication);
  username_ = settings.Value(NetworkProxySettings::kUsername);
  password_ = settings.Value(NetworkProxySettings::kPassword);
  engine_ = settings.BoolValue(NetworkProxySettings::kEngine, NetworkProxySettings::kDefaultEngine);
}

std::string NetworkProxyFactory::Scheme() const {
  return type_ == NetworkProxySettings::ProxyType::Socks5Proxy ? "socks5" : "http";
}

std::string NetworkProxyFactory::SystemProxyFromEnv() {
  const char *keys[] = {"http_proxy", "HTTP_PROXY", "all_proxy", "ALL_PROXY"};
  for (const char *key : keys) {
    if (const char *value = std::getenv(key)) {
      if (value[0]) {
        return value;
      }
    }
  }
  return {};
}

std::string NetworkProxyFactory::ProxyUri() const {
  if (mode_ == Mode::Direct) {
    return {};
  }
  if (mode_ == Mode::System) {
    return SystemProxyFromEnv();
  }
  if (hostname_.empty() || port_ <= 0) {
    return {};
  }
  std::string auth;
  if (use_authentication_ && !username_.empty()) {
    auth = username_ + ":" + password_ + "@";
  }
  return Scheme() + "://" + auth + hostname_ + ":" + std::to_string(port_);
}

GstEngineProxy::Options NetworkProxyFactory::EngineOptions() const {
  return GstEngineProxy::FromSettings(static_cast<NetworkProxySettings::Mode>(static_cast<int>(mode_)), type_, engine_, hostname_,
                                      port_, use_authentication_, username_, password_);
}
