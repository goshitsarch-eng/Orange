#include "device/copytodevicedialog.h"

#include "core/application.h"
#include "device/devicecopy.h"
#include "device/deviceeject.h"
#include "device/devicecopysupported.h"
#include "device/devicemanager.h"
#include "organize/organizedialog.h"
#include "organize/organizetranscode.h"
#include "translations/translations.h"
#include "widgets/freespacebar.h"

#include <adwaita.h>

void CopyToDeviceDialog::Show(GtkWindow *parent, Application *app, const SongList &songs, const std::string &playlist,
                              const std::vector<std::string> &filenames) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Copy to device"));
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  app->device_manager()->Rescan();
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  gtk_box_append(GTK_BOX(box), list);
  if (app->device_manager()->devices().empty()) {
    gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("No devices connected.")));
  }
  for (const ConnectedDevice &device : app->device_manager()->devices()) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), device.friendly_name.c_str());
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), device.mount_path.empty() ? device.backend.c_str() : device.mount_path.c_str());
    GtkWidget *copy = gtk_button_new_with_label(Translations::CStr(DeviceCopy::CopyButtonLabel(!songs.empty(), !filenames.empty())));
    auto *owned = new SongList(songs);
    auto *owned_files = new std::vector<std::string>(filenames);
    auto *owned_device = new ConnectedDevice(device);
    g_object_set_data_full(G_OBJECT(copy), "device", owned_device, [](gpointer p) { delete static_cast<ConnectedDevice *>(p); });
    g_object_set_data_full(G_OBJECT(copy), "songs", owned, [](gpointer p) { delete static_cast<SongList *>(p); });
    g_object_set_data_full(G_OBJECT(copy), "filenames", owned_files, [](gpointer p) { delete static_cast<std::vector<std::string> *>(p); });
    g_object_set_data_full(G_OBJECT(copy), "playlist", g_strdup(playlist.c_str()), g_free);
    g_object_set_data(G_OBJECT(copy), "parent", parent);
    g_signal_connect(copy, "clicked", G_CALLBACK((+[](GtkButton *btn, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       auto *device = static_cast<ConnectedDevice *>(g_object_get_data(G_OBJECT(btn), "device"));
                       auto *songs = static_cast<SongList *>(g_object_get_data(G_OBJECT(btn), "songs"));
                       auto *file_paths = static_cast<std::vector<std::string> *>(g_object_get_data(G_OBJECT(btn), "filenames"));
                       const std::vector<std::string> filenames = file_paths ? *file_paths : std::vector<std::string>{};
                       SongList source = songs && !songs->empty() ? *songs
                                         : filenames.empty() && application->playlist_manager() && application->playlist_manager()->current()
                                               ? application->playlist_manager()->current()->songs()
                                               : SongList{};
                       if (!device || !DeviceCopy::HasOrganizeSource(source, filenames)) {
                         return;
                       }
                       const char *requested = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "playlist"));
                       const std::string playlist_name = DeviceCopyPlaylist::NameForCopy(
                           requested ? requested : "", songs && songs->empty() && filenames.empty(),
                           application->playlist_manager() && application->playlist_manager()->current()
                               ? application->playlist_manager()->current()->name()
                               : std::string());
                       if (!DeviceCopy::ShouldUseOrganizeDialog(*device) || DeviceCopy::ShouldUseRunnerFallback(*device)) {
                         return;
                       }
                       OrganizeDialog::Request request;
                       request.songs = filenames.empty() ? source : SongList{};
                       request.filenames = filenames;
                       request.destination = DeviceManager::MusicPath(*device);
                       request.move = false;
                       const DeviceDatabaseBackend::Device stored = application->device_manager()->StoredDevice(device->unique_id);
                       request.transcode_mode = OrganizeTranscode::FromDeviceMode(
                           stored.id >= 0 ? stored.transcode_mode : DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported);
                       request.transcode_format = stored.id >= 0 ? stored.transcode_format : Song::FileType::MPEG;
                       request.supported_filetypes = DeviceCopySupported::ForDevice(*device);
                       request.show_eject = DeviceEject::Supported(*device);
                       request.device_id = device->unique_id;
                       request.playlist = playlist_name;
                       OrganizeDialog::Show(GTK_WINDOW(g_object_get_data(G_OBJECT(btn), "parent")), application, request);
                     })),
                     app);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), copy);
    gtk_list_box_append(GTK_LIST_BOX(list), row);
    if (!device.mount_path.empty()) {
      auto *space = new FreeSpaceBar();
      space->SetPath(device.mount_path);
      gtk_box_append(GTK_BOX(box), space->widget());
      g_signal_connect(space->widget(), "destroy", G_CALLBACK(+[](GtkWidget *, gpointer data) {
                         delete static_cast<FreeSpaceBar *>(data);
                       }),
                       space);
    }
  }
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
