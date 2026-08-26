#ifndef STRAWBERRY_UDISKS2LISTER_H
#define STRAWBERRY_UDISKS2LISTER_H

#include "device/devicelister.h"

class Udisks2Lister : public DeviceLister {
 public:
  std::string backend() const override { return "udisks2"; }
  std::vector<ConnectedDevice> List() const override;
};

#endif
