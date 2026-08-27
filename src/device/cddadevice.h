#ifndef STRAWBERRY_CDDADEVICE_H
#define STRAWBERRY_CDDADEVICE_H

#include "core/signal.h"
#include "core/song.h"
#include "device/connecteddevice.h"

#include <glib.h>

#include <string>

class CddaDevice {
 public:
  explicit CddaDevice(ConnectedDevice device = {});
  ~CddaDevice();

  CddaDevice(const CddaDevice &) = delete;
  CddaDevice &operator=(const CddaDevice &) = delete;

  const ConnectedDevice &info() const { return device_; }
  SongList Songs() const;
  bool Init();
  void WatchForDiscChanges(bool watch);
  void CheckDiscChanged();
  void AckMediaChanged();
  void set_loader_active(bool active) { loader_active_ = active; }
  bool loader_active() const { return loader_active_; }
  bool watching() const { return watch_id_ != 0; }
  bool has_handle() const { return cdio_ != nullptr; }

  Signal<> DiscChanged;

 private:
  static gboolean OnPoll(gpointer data);

  ConnectedDevice device_;
  void *cdio_ = nullptr;
  guint watch_id_ = 0;
  bool loader_active_ = false;
};

#endif
