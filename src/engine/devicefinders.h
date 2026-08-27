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

  static std::string ChoiceKey(const std::string &output, const std::string &device);
  static void SplitChoiceKey(const std::string &key, std::string *output, std::string *device);
  static std::string OutputLabel(const std::string &output);

 private:
  std::vector<std::unique_ptr<DeviceFinder>> finders_;
  std::vector<AudioDevice> devices_;
};

#endif
