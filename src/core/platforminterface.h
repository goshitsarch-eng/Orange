#ifndef STRAWBERRY_PLATFORMINTERFACE_H
#define STRAWBERRY_PLATFORMINTERFACE_H

#include <string>

class PlatformInterface {
 public:
  virtual ~PlatformInterface() = default;
  virtual void Activate() = 0;
  virtual bool LoadUrl(const std::string &url) = 0;
};

#endif
