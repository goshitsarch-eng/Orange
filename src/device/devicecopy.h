#ifndef STRAWBERRY_DEVICECOPY_H
#define STRAWBERRY_DEVICECOPY_H

#include "core/song.h"
#include "device/connecteddevice.h"
#include "device/devicecopyjob.h"
#include "organize/organizedialog.h"

namespace DeviceCopy {

inline bool IsFilesystemDevice(const ConnectedDevice &device) { return !device.mount_path.empty(); }

inline bool ShouldUseOrganizeDialog(const ConnectedDevice &device) { return DeviceCopyJob::ShouldUseOrganizeDialog(device); }

inline bool CanCopyToCollection(const SongList &songs, bool filesystem = true) { return filesystem && !songs.empty(); }

inline OrganizeDialog::Request CollectionRequest(const SongList &songs) {
  OrganizeDialog::Request request;
  request.songs = songs;
  request.move = false;
  return request;
}

}  // namespace DeviceCopy

#endif
