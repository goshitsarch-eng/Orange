#ifndef STRAWBERRY_DEVICELISTER_H
#define STRAWBERRY_DEVICELISTER_H

#include "device/connecteddevice.h"

#include <vector>

class DeviceLister {
 public:
  virtual ~DeviceLister() = default;
  virtual std::string backend() const = 0;
  virtual std::vector<ConnectedDevice> List() const = 0;
};

#endif
