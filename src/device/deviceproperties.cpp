#include "device/deviceproperties.h"

#include "core/application.h"
#include "device/devicedatabasebackend.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"

#include <adwaita.h>

void DeviceProperties::Show(GtkWindow *parent, Application *app, const ConnectedDevice &device) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Device properties"));
  adw_dialog_set_content_width(dialog, 420);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);

  GtkWidget *name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name), Translations::CStr("Friendly name"));
  gtk_editable_set_text(GTK_EDITABLE(name), device.friendly_name.c_str());
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Name")));
  gtk_box_append(GTK_BOX(box), name);

  GtkWidget *info = gtk_label_new(("Backend: " + device.backend + "\nId: " + device.unique_id + "\nMount: " + device.mount_path +
                                   "\nCapacity: " + FileUtils::PrettySize(device.size))
                                      .c_str());
  gtk_label_set_xalign(GTK_LABEL(info), 0.0f);
  gtk_label_set_wrap(GTK_LABEL(info), TRUE);
  gtk_box_append(GTK_BOX(box), info);

  DeviceDatabaseBackend::Device stored;
  if (app && app->device_manager()) {
    stored = app->device_manager()->StoredDevice(device.unique_id);
  }
  const char *modes[] = {Translations::CStr("Never transcode"), Translations::CStr("Always transcode"),
                         Translations::CStr("Transcode unsupported formats"), nullptr};
  GtkWidget *mode = gtk_drop_down_new_from_strings(modes);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(mode), static_cast<guint>(stored.id >= 0 ? stored.transcode_mode
                                                                                   : DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported));
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Transcode")));
  gtk_box_append(GTK_BOX(box), mode);

  const char *formats[] = {"FLAC", "MP3", "OGG", "MP4", "ALAC", nullptr};
  GtkWidget *format = gtk_drop_down_new_from_strings(formats);
  guint format_index = 0;
  switch (stored.transcode_format) {
    case Song::FileType::MPEG:
      format_index = 1;
      break;
    case Song::FileType::OggVorbis:
      format_index = 2;
      break;
    case Song::FileType::MP4:
      format_index = 3;
      break;
    case Song::FileType::ALAC:
      format_index = 4;
      break;
    default:
      format_index = 0;
      break;
  }
  gtk_drop_down_set_selected(GTK_DROP_DOWN(format), format_index);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Transcode format")));
  gtk_box_append(GTK_BOX(box), format);

  GtkWidget *save = gtk_button_new_with_label(Translations::CStr("Save"));
  gtk_widget_add_css_class(save, "suggested-action");
  auto *owned = new ConnectedDevice(device);
  g_object_set_data_full(G_OBJECT(save), "device", owned, [](gpointer p) { delete static_cast<ConnectedDevice *>(p); });
  g_object_set_data(G_OBJECT(save), "name", name);
  g_object_set_data(G_OBJECT(save), "mode", mode);
  g_object_set_data(G_OBJECT(save), "format", format);
  g_signal_connect(save, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *device = static_cast<ConnectedDevice *>(g_object_get_data(G_OBJECT(button), "device"));
                     if (!application || !device) {
                       return;
                     }
                     const char *name = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "name")));
                     const guint mode = gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "mode")));
                     const guint format = gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "format")));
                     Song::FileType type = Song::FileType::FLAC;
                     switch (format) {
                       case 1:
                         type = Song::FileType::MPEG;
                         break;
                       case 2:
                         type = Song::FileType::OggVorbis;
                         break;
                       case 3:
                         type = Song::FileType::MP4;
                         break;
                       case 4:
                         type = Song::FileType::ALAC;
                         break;
                       default:
                         break;
                     }
                     application->device_manager()->SetDeviceOptions(device->unique_id, name ? name : device->friendly_name,
                                                                     static_cast<DeviceDatabaseBackend::TranscodeMode>(mode), type);
                   }),
                   app);
  gtk_box_append(GTK_BOX(box), save);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, parent ? GTK_WIDGET(parent) : nullptr);
}
