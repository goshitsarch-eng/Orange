#ifndef STRAWBERRY_DEVICECOPY_H
#define STRAWBERRY_DEVICECOPY_H

#include "core/song.h"
#include "device/connecteddevice.h"
#include "device/devicecopyjob.h"
#include "organize/organizedialog.h"

namespace DeviceCopy {

inline bool IsFilesystemDevice(const ConnectedDevice &device) { return !device.mount_path.empty(); }

inline bool UsesDeviceCopyRunner(const ConnectedDevice &device) { return DeviceCopyJob::UsesDeviceCopyRunner(device); }

inline bool ShouldUseOrganizeDialog(const ConnectedDevice &device) { return DeviceCopyJob::ShouldUseOrganizeDialog(device); }

inline bool CanCopyToCollection(const SongList &songs, bool filesystem = true) { return filesystem && !songs.empty(); }

inline OrganizeDialog::Request CollectionRequest(const SongList &songs) {
  OrganizeDialog::Request request;
  request.songs = songs;
  request.move = false;
  return request;
}

}  // namespace DeviceCopy

namespace DeviceCopyPlaylist {

inline std::string NameForCopy(const std::string &requested, const bool copying_whole_playlist, const std::string &current) {
  if (!requested.empty()) {
    return requested;
  }
  return copying_whole_playlist ? current : std::string();
}

inline bool ShouldWriteNamedPlaylist(const std::string &name) { return !name.empty(); }

}  // namespace DeviceCopyPlaylist

#endif
