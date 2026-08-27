#ifndef STRAWBERRY_GIOLISTER_H
#define STRAWBERRY_GIOLISTER_H

#include "device/devicelister.h"

class GioLister : public DeviceLister {
 public:
  std::string backend() const override { return "gio"; }
  std::vector<ConnectedDevice> List() const override;
};

#endif
