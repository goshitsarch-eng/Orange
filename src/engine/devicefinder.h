#ifndef STRAWBERRY_DEVICEFINDER_H
#define STRAWBERRY_DEVICEFINDER_H

#include "engine/enginedevice.h"

#include <string>
#include <vector>

class DeviceFinder {
 public:
  virtual ~DeviceFinder() = default;

  const std::string &name() const { return name_; }
  const std::vector<std::string> &outputs() const { return outputs_; }
  void add_output(const std::string &output) { outputs_.push_back(output); }

  virtual bool Initialize() = 0;
  virtual EngineDeviceList ListDevices() = 0;

 protected:
  DeviceFinder(std::string name, std::vector<std::string> outputs);

 private:
  std::string name_;
  std::vector<std::string> outputs_;
};

#endif
