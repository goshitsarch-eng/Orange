#ifndef STRAWBERRY_ALSADEVICEFINDER_H
#define STRAWBERRY_ALSADEVICEFINDER_H

#include "engine/devicefinder.h"

class AlsaDeviceFinder : public DeviceFinder {
 public:
  AlsaDeviceFinder();
  bool Initialize() override { return true; }
  EngineDeviceList ListDevices() override;
};

#endif
