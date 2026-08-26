#ifndef STRAWBERRY_DEVICEFINDERS_H
#define STRAWBERRY_DEVICEFINDERS_H

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
  void Init();
  std::vector<AudioDevice> ListDevices() const;
  std::vector<std::string> Outputs() const;

 private:
  std::vector<AudioDevice> devices_;
};

#endif
