#ifndef STRAWBERRY_ASIODEVICEFINDER_H
#define STRAWBERRY_ASIODEVICEFINDER_H

#include "engine/devicefinder.h"

class AsioDeviceFinder : public DeviceFinder {
 public:
  AsioDeviceFinder();
  bool Initialize() override { return true; }
  EngineDeviceList ListDevices() override;
};

#endif
