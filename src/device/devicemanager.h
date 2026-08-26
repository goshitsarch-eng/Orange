#ifndef STRAWBERRY_DEVICEMANAGER_H
#define STRAWBERRY_DEVICEMANAGER_H
#include "core/song.h"
#include "core/signal.h"
#include <string>
#include <vector>
struct ConnectedDevice {
  std::string unique_id;
  std::string friendly_name;
  std::string icon;
  int64_t size = 0;
  std::string backend;
};
class DeviceManager {
 public:
  void Init();
  void Rescan();
  const std::vector<ConnectedDevice> &devices() const { return devices_; }
  bool CopySongs(const std::string &device_id, const SongList &songs);
  Signal<> DevicesChanged;
 private:
  std::vector<ConnectedDevice> devices_;
};
#endif
