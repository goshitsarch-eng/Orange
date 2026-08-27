#ifndef STRAWBERRY_UWPDEVICEFINDER_H
#define STRAWBERRY_UWPDEVICEFINDER_H

#include "engine/devicefinder.h"

class UWPDeviceFinder : public DeviceFinder {
 public:
  UWPDeviceFinder();
  bool Initialize() override { return true; }
  EngineDeviceList ListDevices() override;
};

#endif
