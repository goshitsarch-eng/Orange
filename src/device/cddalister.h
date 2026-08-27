#ifndef STRAWBERRY_CDDALISTER_H
#define STRAWBERRY_CDDALISTER_H

#include "device/devicelister.h"

class CddaLister : public DeviceLister {
 public:
  std::string backend() const override { return "cdda"; }
  std::vector<ConnectedDevice> List() const override;
};

#endif
