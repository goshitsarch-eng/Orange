#ifndef STRAWBERRY_BACKENDOUTPUTCHOICES_H
#define STRAWBERRY_BACKENDOUTPUTCHOICES_H

#include "engine/devicefinders.h"

#include <string>
#include <utility>
#include <vector>

namespace BackendOutputChoices {

inline const char *CustomLabel() { return "Custom"; }
inline const char *CustomChoiceKey() { return "__custom__"; }
inline const char *CustomDeviceTitle() { return "Custom device"; }

inline bool IsCustomKey(const std::string &key) { return key == CustomChoiceKey(); }

inline bool DeviceIsListed(const std::string &output, const std::string &device, const std::vector<AudioDevice> &devices) {
  if (device.empty()) {
    return true;
  }
  for (const AudioDevice &entry : devices) {
    if (entry.output == output && entry.id == device) {
      return true;
    }
  }
  return false;
}

inline bool DeviceIsCustom(const std::string &output, const std::string &device, const std::vector<AudioDevice> &devices) {
  return !device.empty() && !DeviceIsListed(output, device, devices);
}

inline void AppendCustom(std::vector<std::pair<std::string, std::string>> *devices) {
  if (!devices) {
    return;
  }
  devices->emplace_back(CustomChoiceKey(), CustomLabel());
}

inline std::string ComboKey(const std::string &output, const std::string &device, const std::vector<AudioDevice> &devices) {
  if (DeviceIsCustom(output, device, devices)) {
    return CustomChoiceKey();
  }
  return DeviceFinders::ChoiceKey(output, device);
}

}  // namespace BackendOutputChoices

#endif
