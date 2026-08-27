#ifndef STRAWBERRY_ALSAPCMDEVICEFINDER_H
#define STRAWBERRY_ALSAPCMDEVICEFINDER_H

#include "engine/devicefinder.h"

class AlsaPCMDeviceFinder : public DeviceFinder {
 public:
  AlsaPCMDeviceFinder();
  bool Initialize() override { return true; }
  EngineDeviceList ListDevices() override;
};

#endif
