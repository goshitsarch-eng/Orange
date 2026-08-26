#ifndef STRAWBERRY_DEVICEVIEWCONTAINER_H
#define STRAWBERRY_DEVICEVIEWCONTAINER_H

#include "device/deviceview.h"

#include <functional>
#include <memory>
#include <string>

#include <gtk/gtk.h>

class DeviceManager;

class DeviceViewContainer {
 public:
  explicit DeviceViewContainer(DeviceManager *manager);

  GtkWidget *widget() const { return widget_; }
  DeviceView *view() { return view_.get(); }
  void Reload();
  void SetSongCallback(std::function<void(const Song &)> callback) { song_cb_ = std::move(callback); }
  void SetAddAllCallback(std::function<void(const SongList &)> callback) { add_all_cb_ = std::move(callback); }

 private:
  void OpenDevice(const std::string &id);

  DeviceManager *manager_ = nullptr;
  GtkWidget *widget_ = nullptr;
  std::unique_ptr<DeviceView> view_;
  std::string browse_id_;
  std::function<void(const Song &)> song_cb_;
  std::function<void(const SongList &)> add_all_cb_;
};

#endif
