#ifndef STRAWBERRY_DEVICEVIEW_H
#define STRAWBERRY_DEVICEVIEW_H

#include "core/song.h"
#include "device/connecteddevice.h"

#include <functional>
#include <string>
#include <vector>

#include <gtk/gtk.h>

class DeviceView {
 public:
  DeviceView();

  GtkWidget *widget() const { return widget_; }
  void ShowDevices(const std::vector<ConnectedDevice> &devices);
  void ShowSongs(const SongList &songs);
  void SetDeviceCallback(std::function<void(const std::string &)> callback) { device_cb_ = std::move(callback); }
  void SetSongCallback(std::function<void(const Song &)> callback) { song_cb_ = std::move(callback); }
  void SetBackCallback(std::function<void()> callback) { back_cb_ = std::move(callback); }
  void SetAddAllCallback(std::function<void()> callback) { add_all_cb_ = std::move(callback); }

 private:
  void Clear();

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  std::function<void(const std::string &)> device_cb_;
  std::function<void(const Song &)> song_cb_;
  std::function<void()> back_cb_;
  std::function<void()> add_all_cb_;
};

#endif
