#ifndef STRAWBERRY_FILESYSTEMDEVICE_H
#define STRAWBERRY_FILESYSTEMDEVICE_H

#include "core/song.h"
#include "device/connecteddevice.h"

#include <string>

class FilesystemDevice {
 public:
  explicit FilesystemDevice(ConnectedDevice device);

  const ConnectedDevice &info() const { return device_; }
  SongList Songs() const;
  bool CopySong(const Song &song, const std::string &destination_dir) const;

 private:
  ConnectedDevice device_;
};

#endif
