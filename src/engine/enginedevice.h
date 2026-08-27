#ifndef STRAWBERRY_ENGINEDEVICE_H
#define STRAWBERRY_ENGINEDEVICE_H

#include <string>
#include <vector>

struct EngineDevice {
  std::string description;
  std::string value;
  std::string iconname = "audio-card-symbolic";
  int card = -1;
  int device = -1;

  std::string GuessIconName() const;
};

using EngineDeviceList = std::vector<EngineDevice>;

#endif
