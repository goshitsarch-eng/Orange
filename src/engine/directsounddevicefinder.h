#ifndef STRAWBERRY_DIRECTSOUNDDEVICEFINDER_H
#define STRAWBERRY_DIRECTSOUNDDEVICEFINDER_H

#include "engine/devicefinder.h"

class DirectSoundDeviceFinder : public DeviceFinder {
 public:
  DirectSoundDeviceFinder();
  bool Initialize() override { return true; }
  EngineDeviceList ListDevices() override;

 private:
#ifdef _WIN32
  struct State {
    EngineDeviceList devices;
  };
  static int __stdcall EnumerateCallback(const void *guid, const char *description, const char *module, void *state);
#endif
};

#endif
