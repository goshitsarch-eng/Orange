#include "device/devicestatefiltermodel.h"

std::vector<ConnectedDevice> DeviceStateFilterModel::Filtered() const {
  if (state_ == State::All) {
    return devices_;
  }
  std::vector<ConnectedDevice> out;
  for (const ConnectedDevice &device : devices_) {
    const bool connected = !device.mount_path.empty();
    if ((state_ == State::Connected && connected) || (state_ == State::Disconnected && !connected)) {
      out.push_back(device);
    }
  }
  return out;
}
