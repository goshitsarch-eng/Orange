#ifndef STRAWBERRY_UWPDEVICEENUM_H
#define STRAWBERRY_UWPDEVICEENUM_H

#include "engine/enginedevice.h"
#include "engine/platformdeviceoutputs.h"

#include <string>

namespace UwpDeviceEnum {

// ABI::Windows::Devices::Enumeration::DeviceClass_AudioRender
inline constexpr int kAudioRenderClass = 2;
inline constexpr const char *kFinderName = "uwpdevice";
inline constexpr const char *kOutput = "wasapi2sink_";

inline bool ShouldInclude(bool enabled) { return enabled; }

inline EngineDevice DefaultDevice() {
  EngineDevice device;
  device.description = PlatformDeviceOutputs::DefaultDeviceDescription();
  device.iconname = "audio-card-symbolic";
  return device;
}

inline EngineDevice FromWinRt(const std::string &id, const std::string &name) {
  EngineDevice device;
  device.value = id;
  device.description = name;
  device.iconname = "audio-card-symbolic";
  return device;
}

}  // namespace UwpDeviceEnum

#endif
