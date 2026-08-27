#ifndef STRAWBERRY_MACOSDEVICEFINDER_H
#define STRAWBERRY_MACOSDEVICEFINDER_H

#include "engine/devicefinder.h"

class MacOsDeviceFinder : public DeviceFinder {
 public:
  MacOsDeviceFinder();
  bool Initialize() override { return true; }
  EngineDeviceList ListDevices() override;
};

#endif
