#ifndef STRAWBERRY_CONNECTEDDEVICE_H
#define STRAWBERRY_CONNECTEDDEVICE_H

#include <cstdint>
#include <string>

struct ConnectedDevice {
  std::string unique_id;
  std::string friendly_name;
  std::string icon;
  int64_t size = 0;
  std::string backend;
  std::string mount_path;
};

#endif
