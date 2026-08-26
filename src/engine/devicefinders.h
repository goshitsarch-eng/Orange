#ifndef STRAWBERRY_DEVICEFINDERS_H
#define STRAWBERRY_DEVICEFINDERS_H

#include "engine/devicefinder.h"
#include "engine/enginedevice.h"

#include <memory>
#include <string>
#include <vector>

struct AudioDevice {
  std::string id;
  std::string description;
  std::string iconname;
  std::string output;
};

class DeviceFinders {
 public:
  DeviceFinders();
  ~DeviceFinders();

  void Init();
  std::vector<DeviceFinder *> ListFinders() const;
  std::vector<AudioDevice> ListDevices() const;
  std::vector<std::string> Outputs() const;

 private:
  std::vector<std::unique_ptr<DeviceFinder>> finders_;
  std::vector<AudioDevice> devices_;
};

#endif
