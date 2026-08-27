#ifndef STRAWBERRY_DEVICEVIEWCONTAINER_H
#define STRAWBERRY_DEVICEVIEWCONTAINER_H

#include "device/devicesongmenu.h"
#include "device/deviceview.h"

#include <functional>
#include <memory>
#include <string>

#include <gtk/gtk.h>

class Application;

class DeviceViewContainer {
 public:
  explicit DeviceViewContainer(Application *app);

  GtkWidget *widget() const { return widget_; }
  DeviceView *view() { return view_.get(); }
  void Reload();
  void SetSongCallback(std::function<void(const Song &)> callback) { song_cb_ = std::move(callback); }
  void SetActivateSongsCallback(std::function<void(const SongList &)> callback) {
    if (view_) {
      view_->SetActivateSongsCallback(std::move(callback));
    }
  }
  void SetEnqueueCallback(std::function<void(const SongList &)> callback) {
    if (view_) {
      view_->SetEnqueueCallback(std::move(callback));
    }
  }
  void SetAddAllCallback(std::function<void(const SongList &)> callback) { add_all_cb_ = std::move(callback); }
  void SetPlaylistCallback(std::function<void(DeviceSongMenu::Action, const SongList &)> callback) {
    playlist_cb_ = std::move(callback);
  }

 private:
  GtkWindow *ParentWindow() const;
  void OpenDevice(const std::string &id);
  void ShowDeviceMenu(const ConnectedDevice &device);
  void ShowSongMenu(const Song &song);
  void ConfirmForget(const std::string &id, const std::string &backend);
  void FinishForget(const std::string &id);
  void ConfirmDelete(const SongList &songs);
  void FinishDelete(const SongList &songs);

  Application *app_ = nullptr;
  GtkWidget *widget_ = nullptr;
  std::unique_ptr<DeviceView> view_;
  std::string browse_id_;
  std::function<void(const Song &)> song_cb_;
  std::function<void(const SongList &)> add_all_cb_;
  std::function<void(DeviceSongMenu::Action, const SongList &)> playlist_cb_;
};

#endif
