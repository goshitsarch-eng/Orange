#include "device/deviceviewcontainer.h"

#include "device/devicemanager.h"

DeviceViewContainer::DeviceViewContainer(DeviceManager *manager) : manager_(manager) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  view_ = std::make_unique<DeviceView>();
  gtk_widget_set_vexpand(view_->widget(), TRUE);
  gtk_box_append(GTK_BOX(widget_), view_->widget());
  view_->SetDeviceCallback([this](const std::string &id) { OpenDevice(id); });
  view_->SetBackCallback([this]() {
    browse_id_.clear();
    Reload();
  });
  view_->SetSongCallback([this](const Song &song) {
    if (song_cb_) song_cb_(song);
  });
  view_->SetAddAllCallback([this]() {
    if (add_all_cb_ && manager_ && !browse_id_.empty()) {
      add_all_cb_(manager_->Songs(browse_id_));
    }
  });
  Reload();
}

void DeviceViewContainer::Reload() {
  if (!manager_) {
    return;
  }
  manager_->Rescan();
  if (browse_id_.empty()) {
    view_->ShowDevices(manager_->devices());
  } else {
    view_->ShowSongs(manager_->Songs(browse_id_));
  }
}

void DeviceViewContainer::OpenDevice(const std::string &id) {
  browse_id_ = id;
  Reload();
}
