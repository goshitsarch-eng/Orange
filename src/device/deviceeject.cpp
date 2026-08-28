#include "device/deviceeject.h"

#include "config.h"

#ifdef HAVE_GIO
#include <gio/gio.h>
#endif

namespace DeviceEject {

#ifdef HAVE_GIO

namespace {

struct UnmountRequest {
  GVolume *volume = nullptr;
  GMount *mount = nullptr;
  GFile *file = nullptr;
  std::function<void()> finished;
};

void UnmountRequestFinish(UnmountRequest *request) {
  if (!request) {
    return;
  }
  if (request->finished) {
    request->finished();
  }
  if (request->volume) {
    g_object_unref(request->volume);
  }
  if (request->mount) {
    g_object_unref(request->mount);
  }
  if (request->file) {
    g_object_unref(request->file);
  }
  delete request;
}

void VolumeEjectFinished(GObject *source, GAsyncResult *result, gpointer data) {
  g_volume_eject_with_operation_finish(G_VOLUME(source), result, nullptr);
  UnmountRequestFinish(static_cast<UnmountRequest *>(data));
}

void MountEjectFinished(GObject *source, GAsyncResult *result, gpointer data) {
  g_mount_eject_with_operation_finish(G_MOUNT(source), result, nullptr);
  UnmountRequestFinish(static_cast<UnmountRequest *>(data));
}

void MountUnmountFinished(GObject *source, GAsyncResult *result, gpointer data) {
  g_mount_unmount_with_operation_finish(G_MOUNT(source), result, nullptr);
  UnmountRequestFinish(static_cast<UnmountRequest *>(data));
}

void FileUnmountFinished(GObject *source, GAsyncResult *result, gpointer data) {
  g_file_unmount_mountable_with_operation_finish(G_FILE(source), result, nullptr);
  UnmountRequestFinish(static_cast<UnmountRequest *>(data));
}

}  // namespace

#endif

bool UnmountPath(const std::string &mount_path, std::function<void()> finished) {
  if (mount_path.empty() || SkipsUnmount(mount_path)) {
    return false;
  }
#ifdef HAVE_GIO
  GVolumeMonitor *monitor = g_volume_monitor_get();
  GMount *match_mount = nullptr;
  GVolume *match_volume = nullptr;
  GList *mounts = g_volume_monitor_get_mounts(monitor);
  for (GList *item = mounts; item; item = item->next) {
    GMount *mount = G_MOUNT(item->data);
    if (GFile *root = g_mount_get_root(mount)) {
      gchar *path = g_file_get_path(root);
      if (path && SameMountPath(path, mount_path)) {
        match_mount = G_MOUNT(g_object_ref(mount));
        match_volume = g_mount_get_volume(mount);
      }
      g_free(path);
      g_object_unref(root);
    }
    if (match_mount) {
      break;
    }
  }
  if (mounts) {
    g_list_free_full(mounts, g_object_unref);
  }
  g_object_unref(monitor);

  const UnmountAction action = ActionFor(false, match_volume != nullptr, match_volume && g_volume_can_eject(match_volume) == TRUE,
                                         match_mount != nullptr, match_mount && g_mount_can_eject(match_mount) == TRUE,
                                         match_mount && g_mount_can_unmount(match_mount) == TRUE, true);
  if (action == UnmountAction::None) {
    if (match_volume) {
      g_object_unref(match_volume);
    }
    if (match_mount) {
      g_object_unref(match_mount);
    }
    return false;
  }

  auto *request = new UnmountRequest;
  request->volume = match_volume;
  request->mount = match_mount;
  request->finished = std::move(finished);
  switch (action) {
    case UnmountAction::VolumeEject:
      g_volume_eject_with_operation(request->volume, G_MOUNT_UNMOUNT_NONE, nullptr, nullptr,
                                    reinterpret_cast<GAsyncReadyCallback>(VolumeEjectFinished), request);
      return true;
    case UnmountAction::MountEject:
      g_mount_eject_with_operation(request->mount, G_MOUNT_UNMOUNT_NONE, nullptr, nullptr,
                                   reinterpret_cast<GAsyncReadyCallback>(MountEjectFinished), request);
      return true;
    case UnmountAction::MountUnmount:
      g_mount_unmount_with_operation(request->mount, G_MOUNT_UNMOUNT_NONE, nullptr, nullptr,
                                     reinterpret_cast<GAsyncReadyCallback>(MountUnmountFinished), request);
      return true;
    case UnmountAction::FileUnmount:
      request->file = g_file_new_for_path(mount_path.c_str());
      g_file_unmount_mountable_with_operation(request->file, G_MOUNT_UNMOUNT_NONE, nullptr, nullptr,
                                              reinterpret_cast<GAsyncReadyCallback>(FileUnmountFinished), request);
      return true;
    case UnmountAction::None:
      break;
  }
  UnmountRequestFinish(request);
  return false;
#else
  (void)finished;
  return false;
#endif
}

}  // namespace DeviceEject
