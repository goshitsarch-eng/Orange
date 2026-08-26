#include "device/deviceviewcontainer.h"

#include "core/application.h"
#include "device/copytodevicedialog.h"
#include "device/devicecopy.h"
#include "device/devicemenu.h"
#include "device/deviceproperties.h"
#include "device/devicedeletedialog.h"
#include "device/deviceforgetdialog.h"
#include "device/deviceconnectdialog.h"
#include "device/deviceviewlook.h"
#include "device/deviceviewreload.h"
#include "device/devicesongmenu.h"
#include "organize/organizedialog.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <string>

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
  if (DeviceViewReload::ShouldRescanOnReload()) {
    app_->device_manager()->Rescan();
  }
  if (browse_id_.empty()) {
    view_->ShowDevices(app_->device_manager()->devices());
  } else {
    const SongList songs = app_->device_manager()->Songs(browse_id_);
    app_->device_manager()->RememberSongCount(browse_id_, static_cast<int>(songs.size()));
    view_->ShowSongs(songs);
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
  if (app_ && app_->device_manager()) {
    for (const ConnectedDevice &device : app_->device_manager()->devices()) {
      if (device.unique_id != id) {
        continue;
      }
      if (DeviceConnectDialog::NeedsMount(device)) {
        app_->device_manager()->Mount(id);
        return;
      }
      const bool stored = app_->device_manager()->StoredDevice(device.unique_id).id != -1;
      if (DeviceConnectDialog::NeedsFirstConnectPrompt(stored, false, device.backend)) {
        AdwDialog *dialog = adw_alert_dialog_new(Translations::CStr(DeviceConnectDialog::Title()), Translations::CStr(DeviceConnectDialog::Message()));
        adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "cancel", Translations::CStr(DeviceConnectDialog::Cancel()));
        adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "connect", Translations::CStr(DeviceConnectDialog::Accept()));
        adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "connect", ADW_RESPONSE_SUGGESTED);
        auto *owned = new std::string(id);
        g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *, const char *response, gpointer data) {
                           auto *self = static_cast<DeviceViewContainer *>(g_object_get_data(G_OBJECT(data), "container"));
                           auto *device_id = static_cast<std::string *>(g_object_get_data(G_OBJECT(data), "device-id"));
                           if (self && device_id && g_strcmp0(response, "connect") == 0) {
                             if (self->app_ && self->app_->device_manager()) {
                               self->app_->device_manager()->Remember(*device_id);
                             }
                             self->browse_id_ = *device_id;
                             self->Reload();
                           }
                         }),
                         dialog);
        g_object_set_data(G_OBJECT(dialog), "container", this);
        g_object_set_data_full(G_OBJECT(dialog), "device-id", owned, [](gpointer p) { delete static_cast<std::string *>(p); });
        adw_dialog_present(dialog, GTK_WIDGET(ParentWindow()));
        return;
      }
      break;
    }
  }
  browse_id_ = id;
  Reload();
}

void DeviceViewContainer::ShowDeviceMenu(const ConnectedDevice &device) {
  if (!app_) {
    return;
  }
  const bool remembered = app_->device_manager()->StoredDevice(device.unique_id).id != -1;
  const DeviceMenu::DeviceState state = DeviceMenu::FromDevice(device, remembered);
  GMenu *menu = g_menu_new();
  for (const DeviceMenu::Item &item : DeviceMenu::VisibleItems(state)) {
    g_menu_append(menu, Translations::CStr(item.label), (std::string("device.") + item.id).c_str());
  }
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
                         self->ConfirmForget(device->unique_id, device->backend);
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
  bool filesystem = true;
  if (app_) {
    for (const ConnectedDevice &device : app_->device_manager()->devices()) {
      if (device.unique_id == browse_id_) {
        filesystem = DeviceCopy::IsFilesystemDevice(device);
        break;
      }
    }
  }
  GMenu *menu = g_menu_new();
  for (const DeviceSongMenu::Item &item : DeviceSongMenu::VisibleItems(songs, filesystem)) {
    g_menu_append(menu, Translations::CStr(item.label), (std::string("devicesong.") + item.id).c_str());
  }
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
                       const DeviceSongMenu::Action action = DeviceSongMenu::FromId(g_action_get_name(G_ACTION(act)));
                       if (DeviceSongMenu::IsPlaylistAction(action)) {
                         if (self->playlist_cb_) {
                           self->playlist_cb_(action, *songs);
                         } else if (self->add_all_cb_) {
                           self->add_all_cb_(*songs);
                         } else if (self->song_cb_) {
                           for (const Song &selected : *songs) {
                             self->song_cb_(selected);
                           }
                         }
                         return;
                       }
                       if (action == DeviceSongMenu::Action::Copy && self->app_) {
                         OrganizeDialog::Show(self->ParentWindow(), self->app_, DeviceCopy::CollectionRequest(*songs));
                       } else if (action == DeviceSongMenu::Action::Delete && self->app_ && !self->browse_id_.empty()) {
                         self->ConfirmDelete(*songs);
                       }
                     }),
                     this);
    g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
  };
  for (const DeviceSongMenu::Item &item : DeviceSongMenu::Items()) {
    add(item.id);
  }
  gtk_widget_insert_action_group(popover, "devicesong", G_ACTION_GROUP(group));
  g_object_set_data_full(G_OBJECT(popover), "songs", owned, [](gpointer p) { delete static_cast<SongList *>(p); });
  gtk_popover_popup(GTK_POPOVER(popover));
}

void DeviceViewContainer::FinishForget(const std::string &id) {
  if (app_ && app_->device_manager()) {
    app_->device_manager()->Forget(id);
  }
  browse_id_.clear();
  Reload();
}

void DeviceViewContainer::ConfirmForget(const std::string &id, const std::string &backend) {
  if (!DeviceForgetDialog::NeedsPrompt(backend)) {
    FinishForget(id);
    return;
  }
  AdwDialog *dialog = adw_alert_dialog_new(Translations::CStr(DeviceForgetDialog::Title()), Translations::CStr(DeviceForgetDialog::Message()));
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "cancel", Translations::CStr(DeviceForgetDialog::Cancel()));
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "forget", Translations::CStr(DeviceForgetDialog::Accept()));
  adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "forget", ADW_RESPONSE_DESTRUCTIVE);
  auto *owned = new std::string(id);
  g_object_set_data(G_OBJECT(dialog), "container", this);
  g_object_set_data_full(G_OBJECT(dialog), "device-id", owned, [](gpointer p) { delete static_cast<std::string *>(p); });
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *, const char *response, gpointer data) {
                     auto *self = static_cast<DeviceViewContainer *>(g_object_get_data(G_OBJECT(data), "container"));
                     auto *device_id = static_cast<std::string *>(g_object_get_data(G_OBJECT(data), "device-id"));
                     if (self && device_id && g_strcmp0(response, "forget") == 0) {
                       self->FinishForget(*device_id);
                     }
                   }),
                   dialog);
  adw_dialog_present(dialog, GTK_WIDGET(ParentWindow()));
}

void DeviceViewContainer::FinishDelete(const SongList &songs) {
  if (!app_ || !app_->device_manager() || browse_id_.empty()) {
    return;
  }
  for (const Song &selected : songs) {
    app_->device_manager()->DeleteSong(browse_id_, selected);
  }
  Reload();
}

void DeviceViewContainer::ConfirmDelete(const SongList &songs) {
  AdwDialog *dialog = adw_alert_dialog_new(Translations::CStr(DeviceDeleteDialog::Title()), Translations::CStr(DeviceDeleteDialog::Message()));
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "cancel", Translations::CStr(DeviceDeleteDialog::Cancel()));
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "delete", Translations::CStr(DeviceDeleteDialog::Accept()));
  adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
  auto *owned = new SongList(songs);
  g_object_set_data(G_OBJECT(dialog), "container", this);
  g_object_set_data_full(G_OBJECT(dialog), "songs", owned, [](gpointer p) { delete static_cast<SongList *>(p); });
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *, const char *response, gpointer data) {
                     auto *self = static_cast<DeviceViewContainer *>(g_object_get_data(G_OBJECT(data), "container"));
                     auto *selected = static_cast<SongList *>(g_object_get_data(G_OBJECT(data), "songs"));
                     if (self && selected && g_strcmp0(response, "delete") == 0) {
                       self->FinishDelete(*selected);
                     }
                   }),
                   dialog);
  adw_dialog_present(dialog, GTK_WIDGET(ParentWindow()));
}
