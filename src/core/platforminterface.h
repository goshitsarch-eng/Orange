#ifndef STRAWBERRY_PLATFORMINTERFACE_H
#define STRAWBERRY_PLATFORMINTERFACE_H

#include <string>

class PlatformInterface {
 public:
  virtual ~PlatformInterface() = default;
  virtual std::string name() const = 0;
  virtual void SetStartup(bool enabled) = 0;
  virtual bool IsStartupEnabled() const = 0;
};

#endif
