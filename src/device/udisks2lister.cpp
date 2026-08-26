#include "device/udisks2lister.h"

#include "config.h"
#include "core/logging.h"

#ifdef HAVE_UDISKS2
#include <gio/gio.h>
#endif

std::vector<ConnectedDevice> Udisks2Lister::List() const {
  std::vector<ConnectedDevice> devices;
#ifdef HAVE_UDISKS2
  GError *error = nullptr;
  GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
  if (!bus) {
    if (error) {
      LogWarning("UDisks2 bus: %s", error->message);
      g_error_free(error);
    }
    return devices;
  }
  GVariant *reply = g_dbus_connection_call_sync(bus, "org.freedesktop.UDisks2", "/org/freedesktop/UDisks2",
                                                "org.freedesktop.DBus.ObjectManager", "GetManagedObjects", nullptr,
                                                G_VARIANT_TYPE("(a{oa{sa{sv}}})"), G_DBUS_CALL_FLAGS_NONE, 4000, nullptr, &error);
  if (!reply) {
    if (error) {
      g_error_free(error);
    }
    g_object_unref(bus);
    return devices;
  }
  GVariant *objects = g_variant_get_child_value(reply, 0);
  GVariantIter obj_iter;
  g_variant_iter_init(&obj_iter, objects);
  const gchar *path = nullptr;
  GVariant *ifaces = nullptr;
  while (g_variant_iter_loop(&obj_iter, "{&o@a{sa{sv}}}", &path, &ifaces)) {
    GVariant *fs = g_variant_lookup_value(ifaces, "org.freedesktop.UDisks2.Filesystem", G_VARIANT_TYPE("a{sv}"));
    if (!fs) {
      continue;
    }
    GVariant *block = g_variant_lookup_value(ifaces, "org.freedesktop.UDisks2.Block", G_VARIANT_TYPE("a{sv}"));
    ConnectedDevice device;
    device.backend = "udisks2";
    device.unique_id = path ? path : "";
    device.icon = "drive-harddisk-usb-symbolic";
    if (block) {
      gchar *id_label = nullptr;
      gchar *device_file = nullptr;
      guint64 size = 0;
      g_variant_lookup(block, "IdLabel", "s", &id_label);
      g_variant_lookup(block, "Device", "^ay", &device_file);
      g_variant_lookup(block, "Size", "t", &size);
      device.friendly_name = id_label && *id_label ? id_label : (device_file ? device_file : "UDisks2 volume");
      device.size = static_cast<int64_t>(size);
      g_free(id_label);
      g_free(device_file);
      g_variant_unref(block);
    } else {
      device.friendly_name = path ? path : "UDisks2 volume";
    }
    GVariant *mounts = g_variant_lookup_value(fs, "MountPoints", G_VARIANT_TYPE("aay"));
    if (mounts) {
      GVariantIter mount_iter;
      g_variant_iter_init(&mount_iter, mounts);
      const gchar *mount = nullptr;
      if (g_variant_iter_loop(&mount_iter, "^ay", &mount) && mount) {
        device.mount_path = mount;
      }
      g_variant_unref(mounts);
    }
    g_variant_unref(fs);
    devices.push_back(device);
  }
  g_variant_unref(objects);
  g_variant_unref(reply);
  g_object_unref(bus);
#endif
  return devices;
}
