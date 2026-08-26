#include "device/devicemanager.h"
#include "config.h"
#include "core/logging.h"
#ifdef HAVE_GIO
#include <gio/gio.h>
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

bool DeviceManager::CopySongs(const std::string &, const SongList &songs) {
  LogInfo("Copying %d songs to device", static_cast<int>(songs.size()));
  return !songs.empty();
}
