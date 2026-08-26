#include "device/deviceviewcontainer.h"

#include "core/application.h"
#include "device/copytodevicedialog.h"
#include "device/deviceproperties.h"
#include "translations/translations.h"

#include <adwaita.h>

DeviceViewContainer::DeviceViewContainer(Application *app) : app_(app) {
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
    if (song_cb_) {
      song_cb_(song);
    }
  });
  view_->SetAddAllCallback([this]() {
    if (add_all_cb_ && app_ && !browse_id_.empty()) {
      add_all_cb_(app_->device_manager()->Songs(browse_id_));
    }
  });
  view_->SetDeviceMenuCallback([this](const ConnectedDevice &device) { ShowDeviceMenu(device); });
  view_->SetSongMenuCallback([this](const Song &song) { ShowSongMenu(song); });
  Reload();
}

void DeviceViewContainer::Reload() {
  if (!app_ || !app_->device_manager()) {
    return;
  }
  app_->device_manager()->Rescan();
  if (browse_id_.empty()) {
    view_->ShowDevices(app_->device_manager()->devices());
  } else {
    view_->ShowSongs(app_->device_manager()->Songs(browse_id_));
  }
}

GtkWindow *DeviceViewContainer::ParentWindow() const {
  if (!widget_) {
    return nullptr;
  }
  GtkRoot *root = gtk_widget_get_root(widget_);
  return root && GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
}

void DeviceViewContainer::OpenDevice(const std::string &id) {
  browse_id_ = id;
  Reload();
}

void DeviceViewContainer::ShowDeviceMenu(const ConnectedDevice &device) {
  if (!app_) {
    return;
  }
  GMenu *menu = g_menu_new();
  g_menu_append(menu, Translations::CStr("Browse"), "device.browse");
  g_menu_append(menu, Translations::CStr("Copy playlist…"), "device.copy");
  g_menu_append(menu, Translations::CStr("Properties…"), "device.properties");
  g_menu_append(menu, Translations::CStr("Unmount"), "device.unmount");
  g_menu_append(menu, Translations::CStr("Forget"), "device.forget");
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, view_->list());
  auto *owned = new ConnectedDevice(device);
  GSimpleActionGroup *group = g_simple_action_group_new();
  auto add = [&](const char *name) {
    GSimpleAction *action = g_simple_action_new(name, nullptr);
    g_object_set_data(G_OBJECT(action), "device", owned);
    g_signal_connect(action, "activate", G_CALLBACK(+[](GSimpleAction *act, GVariant *, gpointer data) {
                       auto *self = static_cast<DeviceViewContainer *>(data);
                       auto *device = static_cast<ConnectedDevice *>(g_object_get_data(G_OBJECT(act), "device"));
                       if (!self || !device || !self->app_) {
                         return;
                       }
                       const char *name = g_action_get_name(G_ACTION(act));
                       if (g_strcmp0(name, "browse") == 0) {
                         self->OpenDevice(device->unique_id);
                       } else if (g_strcmp0(name, "copy") == 0) {
                         SongList songs;
                         if (self->app_->playlist_manager()->current()) {
                           songs = self->app_->playlist_manager()->current()->songs();
                         }
                         CopyToDeviceDialog::Show(self->ParentWindow(), self->app_, songs);
                       } else if (g_strcmp0(name, "properties") == 0) {
                         DeviceProperties::Show(self->ParentWindow(), self->app_, *device);
                       } else if (g_strcmp0(name, "unmount") == 0) {
                         self->app_->device_manager()->Unmount(device->unique_id);
                         self->Reload();
                       } else if (g_strcmp0(name, "forget") == 0) {
                         self->app_->device_manager()->Forget(device->unique_id);
                         self->browse_id_.clear();
                         self->Reload();
                       }
                     }),
                     this);
    g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
  };
  add("browse");
  add("copy");
  add("properties");
  add("unmount");
  add("forget");
  gtk_widget_insert_action_group(popover, "device", G_ACTION_GROUP(group));
  g_object_set_data_full(G_OBJECT(popover), "device", owned, [](gpointer p) { delete static_cast<ConnectedDevice *>(p); });
  gtk_popover_popup(GTK_POPOVER(popover));
}

void DeviceViewContainer::ShowSongMenu(const Song &song) {
  SongList songs = view_ ? view_->SelectedSongs() : SongList{};
  if (songs.empty()) {
    songs.push_back(song);
  }
  GMenu *menu = g_menu_new();
  g_menu_append(menu, Translations::CStr("Add to playlist"), "devicesong.add");
  g_menu_append(menu, Translations::CStr("Delete from device"), "devicesong.delete");
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, view_->list());
  auto *owned = new SongList(songs);
  GSimpleActionGroup *group = g_simple_action_group_new();
  auto add = [&](const char *name) {
    GSimpleAction *action = g_simple_action_new(name, nullptr);
    g_object_set_data(G_OBJECT(action), "songs", owned);
    g_signal_connect(action, "activate", G_CALLBACK(+[](GSimpleAction *act, GVariant *, gpointer data) {
                       auto *self = static_cast<DeviceViewContainer *>(data);
                       auto *songs = static_cast<SongList *>(g_object_get_data(G_OBJECT(act), "songs"));
                       if (!self || !songs) {
                         return;
                       }
                       const char *name = g_action_get_name(G_ACTION(act));
                       if (g_strcmp0(name, "add") == 0) {
                         if (self->add_all_cb_) {
                           self->add_all_cb_(*songs);
                         } else if (self->song_cb_) {
                           for (const Song &selected : *songs) {
                             self->song_cb_(selected);
                           }
                         }
                       } else if (g_strcmp0(name, "delete") == 0 && self->app_ && !self->browse_id_.empty()) {
                         for (const Song &selected : *songs) {
                           self->app_->device_manager()->DeleteSong(self->browse_id_, selected);
                         }
                         self->Reload();
                       }
                     }),
                     this);
    g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
  };
  add("add");
  add("delete");
  gtk_widget_insert_action_group(popover, "devicesong", G_ACTION_GROUP(group));
  g_object_set_data_full(G_OBJECT(popover), "songs", owned, [](gpointer p) { delete static_cast<SongList *>(p); });
  gtk_popover_popup(GTK_POPOVER(popover));
}
