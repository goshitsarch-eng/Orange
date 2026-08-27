#ifndef STRAWBERRY_DEVICEVIEWLOOK_H
#define STRAWBERRY_DEVICEVIEWLOOK_H

#include "collection/collectionitem.h"
#include "device/connecteddevice.h"
#include "fileview/fileviewicons.h"

#include <string>

namespace DeviceViewLook {

constexpr int kIconSize = 32;

enum class Status {
  Remembered,
  NotMounted,
  NotConnected,
  Connected
};

inline const char *FallbackIcon(const std::string &backend) {
  if (backend == "cdda") {
    return "media-optical-symbolic";
  }
  if (backend == "mtp" || backend == "gpod") {
    return "multimedia-player-symbolic";
  }
  return "drive-harddisk-usb-symbolic";
}

inline const char *IconName(const ConnectedDevice &device) {
  if (!device.icon.empty()) {
    return device.icon.c_str();
  }
  return FallbackIcon(device.backend);
}

inline bool IsFilesystemBackend(const std::string &backend) {
  return backend == "gio" || backend == "udisks2" || backend == "filesystem";
}

inline Status InferStatus(const ConnectedDevice &device, bool remembered_only = false) {
  if (remembered_only) {
    return Status::Remembered;
  }
  if (!device.mount_path.empty()) {
    return Status::Connected;
  }
  if (IsFilesystemBackend(device.backend)) {
    return Status::NotMounted;
  }
  return Status::NotConnected;
}

inline std::string SongCountText(int song_count) {
  return std::to_string(song_count) + (song_count == 1 ? " song" : " songs");
}

inline std::string UpdatingText(int percent) {
  if (percent < 0) {
    percent = 0;
  }
  if (percent > 100) {
    percent = 100;
  }
  return "Updating " + std::to_string(percent) + "%...";
}

inline bool ShouldMountOnActivate(const ConnectedDevice &device) {
  return InferStatus(device, device.remembered) == Status::NotMounted;
}

// Qt DeviceView::mouseDoubleClickEvent calls Connect() when the device is not already connected.
inline bool ShouldConnectOnDoubleClick(Status status) { return status != Status::Connected; }

inline std::string StatusText(const ConnectedDevice &device, int song_count = -1, bool remembered_only = false) {
  switch (InferStatus(device, remembered_only)) {
    case Status::Remembered:
      return "Not connected";
    case Status::NotMounted:
      return "Not mounted - double click to mount";
    case Status::NotConnected:
      return "Double click to open";
    case Status::Connected:
      if (song_count >= 0) {
        return SongCountText(song_count);
      }
      return device.mount_path;
  }
  return "Double click to open";
}

inline std::string RowStatusText(const ConnectedDevice &device, int song_count = -1, int updating_percent = -1, bool remembered_only = false) {
  const int percent = updating_percent >= 0 ? updating_percent : device.updating_percent;
  if (percent >= 0) {
    return UpdatingText(percent);
  }
  return StatusText(device, song_count >= 0 ? song_count : device.song_count, remembered_only || device.remembered);
}

inline const char *ItemIconName(const CollectionItem *item) {
  if (!item) {
    return FileViewIcons::AudioIcon();
  }
  if (item->type == CollectionItem::Type::Song) {
    const std::string &path = item->metadata.url().empty() ? item->metadata.basefilename() : item->metadata.url();
    return FileViewIcons::IconName(false, path);
  }
  return FileViewIcons::FolderIcon();
}

}  // namespace DeviceViewLook

#endif
