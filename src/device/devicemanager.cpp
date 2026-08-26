#include "device/devicemanager.h"
#include "config.h"
#include "core/logging.h"
#include "utilities/fileutils.h"
#ifdef HAVE_GIO
#include <gio/gio.h>
#include <glib/gstdio.h>
#endif
#ifdef HAVE_MTP
#include <libmtp.h>
#include <cstdlib>
#endif

void DeviceManager::Init() { Rescan(); }

void DeviceManager::Rescan() {
  devices_.clear();
#ifdef HAVE_GIO
  GVolumeMonitor *monitor = g_volume_monitor_get();
  GList *volumes = g_volume_monitor_get_volumes(monitor);
  for (GList *l = volumes; l; l = l->next) {
    GVolume *volume = G_VOLUME(l->data);
    gchar *name = g_volume_get_name(volume);
    gchar *id = g_volume_get_identifier(volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    ConnectedDevice device;
    device.friendly_name = name ? name : "Volume";
    device.unique_id = id ? id : device.friendly_name;
    device.icon = "drive-harddisk-usb-symbolic";
    device.backend = "gio";
    if (GMount *mount = g_volume_get_mount(volume)) {
      if (GFile *root = g_mount_get_root(mount)) {
        gchar *path = g_file_get_path(root);
        if (path) {
          device.mount_path = path;
          g_free(path);
        }
        g_object_unref(root);
      }
      g_object_unref(mount);
    }
    devices_.push_back(device);
    g_free(name);
    g_free(id);
    g_object_unref(volume);
  }
  g_list_free(volumes);
  g_object_unref(monitor);
#endif
#ifdef HAVE_MTP
  {
    LIBMTP_raw_device_t *raw = nullptr;
    int count = 0;
    LIBMTP_Init();
    if (LIBMTP_Detect_Raw_Devices(&raw, &count) == LIBMTP_ERROR_NONE && raw) {
      for (int i = 0; i < count; ++i) {
        LIBMTP_mtpdevice_t *device = LIBMTP_Open_Raw_Device_Uncached(&raw[i]);
        ConnectedDevice entry;
        entry.backend = "mtp";
        entry.icon = "multimedia-player-symbolic";
        if (device) {
          char *name = LIBMTP_Get_Friendlyname(device);
          char *serial = LIBMTP_Get_Serialnumber(device);
          entry.friendly_name = name && *name ? name : "MTP device";
          entry.unique_id = std::string("mtp:") + (serial ? serial : std::to_string(i));
          free(name);
          free(serial);
          LIBMTP_Release_Device(device);
        } else {
          entry.friendly_name = "MTP device";
          entry.unique_id = "mtp:" + std::to_string(i);
        }
        devices_.push_back(entry);
      }
      free(raw);
    }
  }
#endif
#ifdef HAVE_GPOD
  for (ConnectedDevice &device : devices_) {
    if (device.mount_path.empty()) {
      continue;
    }
    if (FileUtils::Exists(FileUtils::Join(device.mount_path, "iPod_Control")) ||
        FileUtils::Exists(FileUtils::Join(device.mount_path, "iTunes_Control"))) {
      device.backend = "gpod";
      device.icon = "multimedia-player-symbolic";
      if (device.friendly_name.find("iPod") == std::string::npos) {
        device.friendly_name += " (iPod)";
      }
    }
  }
#endif
#ifdef HAVE_AUDIOCD
  ConnectedDevice cd;
  cd.friendly_name = "Audio CD";
  cd.unique_id = "cdda";
  cd.icon = "media-optical-symbolic";
  cd.backend = "cdda";
  devices_.push_back(cd);
#endif
  DevicesChanged.Emit();
}

bool DeviceManager::CopySongs(const std::string &device_id, const SongList &songs) {
  ConnectedDevice target;
  for (const ConnectedDevice &device : devices_) {
    if (device.unique_id == device_id || device.friendly_name == device_id) {
      target = device;
      break;
    }
  }
#ifdef HAVE_MTP
  if (target.backend == "mtp") {
    LIBMTP_raw_device_t *raw = nullptr;
    int count = 0;
    if (LIBMTP_Detect_Raw_Devices(&raw, &count) != LIBMTP_ERROR_NONE || !raw) {
      return false;
    }
    LIBMTP_mtpdevice_t *mtp = nullptr;
    for (int i = 0; i < count; ++i) {
      LIBMTP_mtpdevice_t *device = LIBMTP_Open_Raw_Device_Uncached(&raw[i]);
      if (!device) {
        continue;
      }
      char *serial = LIBMTP_Get_Serialnumber(device);
      const std::string id = std::string("mtp:") + (serial ? serial : "");
      free(serial);
      if (id == target.unique_id || count == 1) {
        mtp = device;
        break;
      }
      LIBMTP_Release_Device(device);
    }
    free(raw);
    if (!mtp) {
      return false;
    }
    int copied = 0;
    for (const Song &song : songs) {
      const std::string src = FileUtils::PathFromUri(song.url());
      if (src.empty() || !FileUtils::Exists(src)) {
        continue;
      }
      LIBMTP_file_t *file = LIBMTP_new_file_t();
      file->filename = strdup(FileUtils::BaseName(src).c_str());
      file->filesize = static_cast<uint64_t>(song.filesize() > 0 ? song.filesize() : 0);
      file->parent_id = 0;
      file->storage_id = 0;
      if (LIBMTP_Send_File_From_File(mtp, src.c_str(), file, nullptr, nullptr) == 0) {
        ++copied;
      }
      LIBMTP_destroy_file_t(file);
    }
    LIBMTP_Release_Device(mtp);
    LogInfo("Copied %d songs to MTP device", copied);
    return copied > 0;
  }
#endif
  if (target.mount_path.empty()) {
    LogInfo("Device %s has no mount path", device_id.c_str());
    return false;
  }
#ifdef HAVE_GIO
  const std::string music = FileUtils::Join(target.mount_path, target.backend == "gpod" ? "iPod_Control/Music" : "Music");
  g_mkdir_with_parents(music.c_str(), 0755);
  int copied = 0;
  for (const Song &song : songs) {
    const std::string src = FileUtils::PathFromUri(song.url());
    if (src.empty() || !FileUtils::Exists(src)) {
      continue;
    }
    const std::string dest = FileUtils::Join(music, FileUtils::BaseName(src));
    if (FileUtils::CopyFile(src, dest)) {
      ++copied;
    }
  }
  LogInfo("Copied %d songs to %s", copied, music.c_str());
  return copied > 0;
#else
  (void)songs;
  return false;
#endif
}
