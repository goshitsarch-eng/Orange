#include "device/devicemanager.h"
#include "config.h"
#include "core/logging.h"
#include "utilities/fileutils.h"
#ifdef HAVE_GIO
#include <gio/gio.h>
#include <glib/gstdio.h>
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
  ConnectedDevice mtp;
  mtp.friendly_name = "MTP devices";
  mtp.unique_id = "mtp";
  mtp.icon = "multimedia-player-symbolic";
  mtp.backend = "mtp";
  devices_.push_back(mtp);
#endif
#ifdef HAVE_GPOD
  ConnectedDevice ipod;
  ipod.friendly_name = "iPod";
  ipod.unique_id = "gpod";
  ipod.icon = "multimedia-player-symbolic";
  ipod.backend = "gpod";
  devices_.push_back(ipod);
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
  std::string dest_root;
  for (const ConnectedDevice &device : devices_) {
    if (device.unique_id == device_id || device.friendly_name == device_id) {
      dest_root = device.mount_path;
      break;
    }
  }
  if (dest_root.empty()) {
    LogInfo("Device %s has no mount path", device_id.c_str());
    return false;
  }
#ifdef HAVE_GIO
  const std::string music = FileUtils::Join(dest_root, "Music");
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
