#include "device/cddadevice.h"

#include "config.h"
#include "device/cddadiscchange.h"
#include "device/cddahelpers.h"
#include "device/cddasongloader.h"

#ifdef HAVE_AUDIOCD
#include <cdio/cdio.h>
#endif

CddaDevice::CddaDevice(ConnectedDevice device) : device_(std::move(device)) {
  if (device_.backend.empty()) {
    device_.backend = "cdda";
  }
  if (device_.friendly_name.empty()) {
    device_.friendly_name = "Audio CD";
  }
}

CddaDevice::~CddaDevice() {
  WatchForDiscChanges(false);
#ifdef HAVE_AUDIOCD
  if (cdio_) {
    cdio_destroy(static_cast<CdIo_t *>(cdio_));
    cdio_ = nullptr;
  }
#endif
}

SongList CddaDevice::Songs() const { return CddaSongLoader::LoadDevice(device_.mount_path); }

bool CddaDevice::Init() {
#ifdef HAVE_AUDIOCD
  CddaHelpers::EnsureInit();
  if (!cdio_) {
    const char *path = device_.mount_path.empty() ? nullptr : device_.mount_path.c_str();
    cdio_ = cdio_open(path, DRIVER_DEVICE);
    if (!cdio_) {
      return false;
    }
  }
  WatchForDiscChanges(true);
  return true;
#else
  return false;
#endif
}

void CddaDevice::WatchForDiscChanges(bool watch) {
  if (CddaDiscChange::ShouldStartWatch(watch, watch_id_ != 0)) {
    watch_id_ = g_timeout_add(CddaDiscChange::kPollMs, OnPoll, this);
  } else if (CddaDiscChange::ShouldStopWatch(watch, watch_id_ != 0)) {
    g_source_remove(watch_id_);
    watch_id_ = 0;
  }
}

gboolean CddaDevice::OnPoll(gpointer data) {
  static_cast<CddaDevice *>(data)->CheckDiscChanged();
  return G_SOURCE_CONTINUE;
}

void CddaDevice::CheckDiscChanged() {
#ifdef HAVE_AUDIOCD
  if (!CddaDiscChange::ShouldCheck(cdio_ != nullptr, loader_active_)) {
    return;
  }
  if (CddaDiscChange::MediaChanged(cdio_get_media_changed(static_cast<CdIo_t *>(cdio_)))) {
    DiscChanged.Emit();
  }
#else
  (void)0;
#endif
}

void CddaDevice::AckMediaChanged() {
#ifdef HAVE_AUDIOCD
  if (cdio_) {
    (void)cdio_get_media_changed(static_cast<CdIo_t *>(cdio_));
  }
#endif
}
