#include "device/cddadevice.h"

#include "device/cddasongloader.h"

CddaDevice::CddaDevice(ConnectedDevice device) : device_(std::move(device)) {
  if (device_.backend.empty()) {
    device_.backend = "cdda";
  }
  if (device_.friendly_name.empty()) {
    device_.friendly_name = "Audio CD";
  }
}

SongList CddaDevice::Songs() const { return CddaSongLoader::LoadDevice(device_.mount_path); }
