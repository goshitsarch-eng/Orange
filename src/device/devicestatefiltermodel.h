#ifndef STRAWBERRY_DEVICESTATEFILTERMODEL_H
#define STRAWBERRY_DEVICESTATEFILTERMODEL_H

#include "device/connecteddevice.h"

#include <string>
#include <vector>

class DeviceStateFilterModel {
 public:
  enum class State { All, Connected, Disconnected };
  void SetDevices(const std::vector<ConnectedDevice> &devices) { devices_ = devices; }
  void set_state(State state) { state_ = state; }
  State state() const { return state_; }
  std::vector<ConnectedDevice> Filtered() const;

 private:
  std::vector<ConnectedDevice> devices_;
  State state_ = State::All;
};

#endif
