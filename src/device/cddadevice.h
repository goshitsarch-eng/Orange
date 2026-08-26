#ifndef STRAWBERRY_CDDADEVICE_H
#define STRAWBERRY_CDDADEVICE_H

#include "core/song.h"
#include "device/connecteddevice.h"

#include <string>

class CddaDevice {
 public:
  explicit CddaDevice(ConnectedDevice device = {});

  const ConnectedDevice &info() const { return device_; }
  SongList Songs() const;

 private:
  ConnectedDevice device_;
};

#endif
