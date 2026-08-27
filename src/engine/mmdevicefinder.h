#ifndef STRAWBERRY_MMDEVICEFINDER_H
#define STRAWBERRY_MMDEVICEFINDER_H

#include "engine/devicefinder.h"

class MMDeviceFinder : public DeviceFinder {
 public:
  MMDeviceFinder();
  bool Initialize() override { return true; }
  EngineDeviceList ListDevices() override;
};

#endif
