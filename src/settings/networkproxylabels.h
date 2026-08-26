#ifndef STRAWBERRY_NETWORKPROXYLABELS_H
#define STRAWBERRY_NETWORKPROXYLABELS_H

#include "constants/networkproxysettings.h"

#include <string>
#include <utility>
#include <vector>

namespace NetworkProxyLabels {

inline const char *PageTitle() { return "Network Proxy"; }
inline const char *SystemMode() { return "Use the system proxy settings"; }
inline const char *DirectMode() { return "Direct internet connection"; }
inline const char *ManualMode() { return "Manual proxy configuration"; }
inline const char *HttpType() { return "HTTP proxy"; }
inline const char *SocksType() { return "SOCKS proxy"; }
inline const char *AuthTitle() { return "Use authentication"; }
inline const char *Username() { return "Username"; }
inline const char *Password() { return "Password"; }
inline const char *Port() { return "Port"; }
inline const char *EngineLabel() { return "Use proxy settings for streaming"; }
inline const char *EngineTooltip() { return "Only HTTP proxy is supported for streaming."; }

inline bool ManualEnabled(int mode) { return mode == static_cast<int>(NetworkProxySettings::Mode::Manual); }

inline bool ManualEnabled(const std::string &mode) { return mode == "2"; }

inline std::vector<std::pair<std::string, std::string>> ModeChoices() {
  return {{"0", SystemMode()}, {"1", DirectMode()}, {"2", ManualMode()}};
}

inline std::vector<std::pair<std::string, std::string>> TypeChoices() {
  return {{std::to_string(static_cast<int>(NetworkProxySettings::ProxyType::HttpProxy)), HttpType()},
          {std::to_string(static_cast<int>(NetworkProxySettings::ProxyType::Socks5Proxy)), SocksType()}};
}

inline int ComboIndexFromType(NetworkProxySettings::ProxyType type) {
  return type == NetworkProxySettings::ProxyType::Socks5Proxy ? 1 : 0;
}

inline NetworkProxySettings::ProxyType TypeFromComboIndex(int index) {
  return index == 1 ? NetworkProxySettings::ProxyType::Socks5Proxy : NetworkProxySettings::ProxyType::HttpProxy;
}

}  // namespace NetworkProxyLabels

#endif  // STRAWBERRY_NETWORKPROXYLABELS_H
