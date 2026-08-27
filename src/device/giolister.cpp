#include "device/giolister.h"

#include "config.h"
#include "device/giodevicefilter.h"

#ifdef HAVE_GIO
#include <gio/gio.h>
#endif

std::vector<ConnectedDevice> GioLister::List() const {
  std::vector<ConnectedDevice> devices;
#ifdef HAVE_GIO
  GVolumeMonitor *monitor = g_volume_monitor_get();
  GList *volumes = g_volume_monitor_get_volumes(monitor);
  for (GList *l = volumes; l; l = l->next) {
    GVolume *volume = G_VOLUME(l->data);
    GDrive *drive = g_volume_get_drive(volume);
    const bool removable = drive && g_drive_is_removable(drive) == TRUE;
    const bool system_internal = drive && !removable && g_drive_can_eject(drive) == FALSE;
    gchar *fs = nullptr;
    if (GMount *mounted = g_volume_get_mount(volume)) {
      if (GFile *root = g_mount_get_root(mounted)) {
        GFileInfo *info = g_file_query_filesystem_info(root, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE, nullptr, nullptr);
        if (info) {
          fs = g_strdup(g_file_info_get_attribute_string(info, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE));
          g_object_unref(info);
        }
        g_object_unref(root);
      }
      g_object_unref(mounted);
    }
    const bool suitable = GioDeviceFilter::IsSuitable(true, system_internal, drive != nullptr, removable, fs);
    g_free(fs);
    if (drive) {
      g_object_unref(drive);
    }
    if (!suitable) {
      g_object_unref(volume);
      continue;
    }
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
    devices.push_back(device);
    g_free(name);
    g_free(id);
    g_object_unref(volume);
  }
  g_list_free(volumes);
  g_object_unref(monitor);
#endif
  return devices;
}
