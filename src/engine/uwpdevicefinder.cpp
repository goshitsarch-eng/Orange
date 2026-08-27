#include "engine/uwpdevicefinder.h"

#include "engine/platformdeviceoutputs.h"

UWPDeviceFinder::UWPDeviceFinder() : DeviceFinder("uwpdevice", {"wasapi2sink_"}) {}

EngineDeviceList UWPDeviceFinder::ListDevices() {
#if defined(_WIN32) && defined(_MSC_VER)
  // WinRT device enumeration matches Qt UWPDeviceFinder (MSVC only).
  EngineDeviceList devices;
  EngineDevice fallback;
  fallback.description = PlatformDeviceOutputs::DefaultDeviceDescription();
  fallback.iconname = fallback.GuessIconName();
  devices.push_back(fallback);
  return devices;
#else
  return {};
#endif
}
