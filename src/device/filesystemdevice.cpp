#include "device/filesystemdevice.h"

#include "device/devicemanager.h"
#include "utilities/fileutils.h"

FilesystemDevice::FilesystemDevice(ConnectedDevice device) : device_(std::move(device)) {}

SongList FilesystemDevice::Songs() const { return DeviceManager::SongsFromDirectory(device_.mount_path); }

bool FilesystemDevice::CopySong(const Song &song, const std::string &destination_dir) const {
  const std::string src = FileUtils::PathFromUri(song.url());
  if (!FileUtils::IsFile(src) || destination_dir.empty()) {
    return false;
  }
  return FileUtils::CopyFile(src, FileUtils::Join(destination_dir, FileUtils::BaseName(src)));
}
