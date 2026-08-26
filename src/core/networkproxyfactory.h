#ifndef STRAWBERRY_NETWORKPROXYFACTORY_H
#define STRAWBERRY_NETWORKPROXYFACTORY_H

#include <string>

class NetworkProxyFactory {
 public:
  enum class Mode { System, Direct, Manual };

  void ReloadSettings();
  Mode mode() const { return mode_; }
  std::string ProxyUri() const;

 private:
  Mode mode_ = Mode::System;
  std::string hostname_;
  int port_ = 0;
};

#endif
