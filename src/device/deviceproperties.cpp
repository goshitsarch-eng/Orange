#include "config.h"

#include "device/deviceproperties.h"

#include "core/application.h"
#include "device/deviceconnectdialog.h"
#include "device/devicecopyjob.h"
#include "device/devicedatabasebackend.h"
#include "device/devicepropertiesicons.h"
#include "device/devicepropertiesinfo.h"
#include "device/devicepropertieslabels.h"
#include "device/devicesupportedformats.h"
#include "device/mtpconnection.h"
#include "organize/organizetranscode.h"
#include "translations/translations.h"
#include "widgets/freespacebar.h"

#include <adwaita.h>
#include <gio/gio.h>

#include <memory>
#include <string>
#include <vector>

namespace {

struct FormatsWidgets {
  GtkWidget *stack = nullptr;
  GtkWidget *supported_box = nullptr;
  GtkWidget *supported_list = nullptr;
  GtkWidget *unsupported = nullptr;
  GtkWidget *never = nullptr;
  GtkWidget *format = nullptr;
};

struct FormatsJob {
  std::shared_ptr<bool> alive;
  FormatsWidgets widgets;
  std::string serial;
  std::vector<Song::FileType> types;
  bool ok = false;
  bool has_saved = false;
  Song::FileType stored = Song::FileType::Unknown;
};

void FillSupportedList(GtkWidget *list, const std::vector<Song::FileType> &supported) {
  GtkWidget *child = gtk_widget_get_first_child(list);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list), child);
    child = next;
  }
  for (const std::string &name : DevicePropertiesLabels::SupportedFormatNames(supported)) {
    GtkWidget *row = gtk_label_new(name.c_str());
    gtk_label_set_xalign(GTK_LABEL(row), 0.0f);
    gtk_widget_set_margin_start(row, 8);
    gtk_widget_set_margin_end(row, 8);
    gtk_widget_set_margin_top(row, 4);
    gtk_widget_set_margin_bottom(row, 4);
    gtk_list_box_append(GTK_LIST_BOX(list), row);
  }
}

void ApplyFormats(const FormatsWidgets &widgets, const std::vector<Song::FileType> &supported, bool has_saved, Song::FileType stored) {
  const bool has_supported = !supported.empty();
  gtk_widget_set_sensitive(widgets.unsupported, DevicePropertiesLabels::UnsupportedEnabled(has_supported) ? TRUE : FALSE);
  if (DevicePropertiesLabels::ShouldFallbackToNever(gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets.unsupported)) == TRUE,
                                                    has_supported)) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets.never), TRUE);
  }
  FillSupportedList(widgets.supported_list, supported);
  gtk_widget_set_visible(widgets.supported_box, DevicePropertiesLabels::SupportedListVisible(has_supported) ? TRUE : FALSE);
  Song::FileType preferred = stored;
  if (DevicePropertiesLabels::ShouldPickBestFormat(has_saved, preferred)) {
    preferred = OrganizeTranscode::PickBestFormat(supported);
  }
  gtk_drop_down_set_selected(GTK_DROP_DOWN(widgets.format), static_cast<guint>(DevicePropertiesLabels::IndexOfFormat(preferred)));
}

void ShowFormatsPage(GtkWidget *stack, DeviceSupportedFormats::Page page) {
  gtk_stack_set_visible_child_name(GTK_STACK(stack), DeviceSupportedFormats::StackName(page));
}

}  // namespace

void DeviceProperties::Show(GtkWindow *parent, Application *app, const ConnectedDevice &device) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr(DevicePropertiesLabels::Title()));
  adw_dialog_set_content_width(dialog, 520);

  GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(outer, 16);
  gtk_widget_set_margin_end(outer, 16);
  gtk_widget_set_margin_top(outer, 16);
  gtk_widget_set_margin_bottom(outer, 16);

  GtkWidget *notebook = gtk_notebook_new();

  GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(info_box, 8);
  gtk_widget_set_margin_end(info_box, 8);
  gtk_widget_set_margin_top(info_box, 8);
  gtk_widget_set_margin_bottom(info_box, 8);

  gtk_box_append(GTK_BOX(info_box), gtk_label_new(Translations::CStr(DevicePropertiesLabels::Name())));
  GtkWidget *name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name), Translations::CStr("Friendly name"));
  gtk_editable_set_text(GTK_EDITABLE(name), device.friendly_name.c_str());
  gtk_box_append(GTK_BOX(info_box), name);

  gtk_box_append(GTK_BOX(info_box), gtk_label_new(Translations::CStr(DevicePropertiesLabels::Icon())));
  GtkWidget *icons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  std::vector<std::string> icon_names;
  for (const char *icon_name : DevicePropertiesIcons::Names()) {
    icon_names.emplace_back(icon_name);
  }
  if (!device.icon.empty() && DevicePropertiesIcons::IndexOf(device.icon) < 0) {
    icon_names.push_back(device.icon);
  }
  const std::string current_icon = device.icon.empty() ? DevicePropertiesIcons::Names().front() : device.icon;
  auto *selected_icon = new std::string(DevicePropertiesIcons::EffectiveIcon(current_icon));
  if (!device.icon.empty() && DevicePropertiesIcons::IndexOf(device.icon) < 0) {
    *selected_icon = device.icon;
  }
  GtkWidget *first_icon = nullptr;
  for (const std::string &icon_name : icon_names) {
    GtkWidget *button = gtk_toggle_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(button), DevicePropertiesIcons::GtkName(icon_name.c_str()));
    gtk_widget_set_tooltip_text(button, icon_name.c_str());
    gtk_widget_set_size_request(button, 48, 48);
    g_object_set_data_full(G_OBJECT(button), "icon-name", g_strdup(icon_name.c_str()), g_free);
    if (!first_icon) {
      first_icon = button;
    } else {
      gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(button), GTK_TOGGLE_BUTTON(first_icon));
    }
    if (icon_name == *selected_icon) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    }
    g_signal_connect(button, "toggled", G_CALLBACK(+[](GtkToggleButton *toggle, gpointer data) {
                       if (!gtk_toggle_button_get_active(toggle)) {
                         return;
                       }
                       const char *icon_name = static_cast<const char *>(g_object_get_data(G_OBJECT(toggle), "icon-name"));
                       if (icon_name) {
                         *static_cast<std::string *>(data) = icon_name;
                       }
                     }),
                     selected_icon);
    gtk_box_append(GTK_BOX(icons), button);
  }
  gtk_box_append(GTK_BOX(info_box), icons);

  auto *space_bar = new FreeSpaceBar();
  const DevicePropertiesInfo::Space space = DevicePropertiesInfo::SpaceFor(device);
  if (space.available) {
    space_bar->SetBytes(space.total - space.free, 0, space.total);
  } else {
    gtk_widget_set_visible(space_bar->widget(), FALSE);
  }
  gtk_box_append(GTK_BOX(info_box), space_bar->widget());
  g_object_set_data_full(G_OBJECT(dialog), "freespace", space_bar, [](gpointer p) { delete static_cast<FreeSpaceBar *>(p); });

  GtkWidget *hardware_title = gtk_label_new(Translations::CStr(DevicePropertiesLabels::HardwareInformation()));
  gtk_widget_set_halign(hardware_title, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(info_box), hardware_title);
  if (DevicePropertiesInfo::HasHardwareInfo(device)) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    int row = 0;
    for (const DevicePropertiesInfo::Row &info : DevicePropertiesInfo::Rows(device)) {
      GtkWidget *key = gtk_label_new(Translations::CStr(info.key.c_str()));
      gtk_widget_add_css_class(key, "dim-label");
      gtk_label_set_xalign(GTK_LABEL(key), 0.0f);
      GtkWidget *value = gtk_label_new(info.value.c_str());
      gtk_label_set_xalign(GTK_LABEL(value), 0.0f);
      gtk_label_set_selectable(GTK_LABEL(value), TRUE);
      gtk_grid_attach(GTK_GRID(grid), key, 0, row, 1, 1);
      gtk_grid_attach(GTK_GRID(grid), value, 1, row, 1, 1);
      ++row;
    }
    gtk_box_append(GTK_BOX(info_box), grid);
  } else {
    GtkWidget *missing = gtk_label_new(Translations::CStr(DevicePropertiesLabels::NotConnected()));
    gtk_label_set_wrap(GTK_LABEL(missing), TRUE);
    gtk_label_set_xalign(GTK_LABEL(missing), 0.0f);
    gtk_box_append(GTK_BOX(info_box), missing);
  }

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), info_box, gtk_label_new(Translations::CStr(DevicePropertiesLabels::InformationTab())));

  GtkWidget *formats_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(formats_box, 8);
  gtk_widget_set_margin_end(formats_box, 8);
  gtk_widget_set_margin_top(formats_box, 8);
  gtk_widget_set_margin_bottom(formats_box, 8);

  GtkWidget *supported_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  GtkWidget *supported_title = gtk_label_new(Translations::CStr(DevicePropertiesLabels::SupportedFormats()));
  gtk_widget_set_halign(supported_title, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(supported_box), supported_title);
  GtkWidget *supported_intro = gtk_label_new(Translations::CStr(DevicePropertiesLabels::SupportedFormatsIntro()));
  gtk_label_set_wrap(GTK_LABEL(supported_intro), TRUE);
  gtk_label_set_xalign(GTK_LABEL(supported_intro), 0.0f);
  gtk_box_append(GTK_BOX(supported_box), supported_intro);
  GtkWidget *supported_list = gtk_list_box_new();
  gtk_widget_add_css_class(supported_list, "boxed-list");
  gtk_box_append(GTK_BOX(supported_box), supported_list);
  gtk_box_append(GTK_BOX(formats_box), supported_box);

  GtkWidget *intro = gtk_label_new(Translations::CStr(DevicePropertiesLabels::TranscodeIntro()));
  gtk_label_set_wrap(GTK_LABEL(intro), TRUE);
  gtk_label_set_xalign(GTK_LABEL(intro), 0.0f);
  gtk_box_append(GTK_BOX(formats_box), intro);

  DeviceDatabaseBackend::Device stored;
  if (app && app->device_manager()) {
    stored = app->device_manager()->StoredDevice(device.unique_id);
  }
  const DeviceDatabaseBackend::TranscodeMode mode =
      stored.id >= 0 ? stored.transcode_mode : DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported;
  GtkWidget *never = gtk_check_button_new_with_label(Translations::CStr(DevicePropertiesLabels::Never()));
  GtkWidget *unsupported = gtk_check_button_new_with_label(Translations::CStr(DevicePropertiesLabels::Unsupported()));
  gtk_check_button_set_group(GTK_CHECK_BUTTON(unsupported), GTK_CHECK_BUTTON(never));
  GtkWidget *always = gtk_check_button_new_with_label(Translations::CStr(DevicePropertiesLabels::Always()));
  gtk_check_button_set_group(GTK_CHECK_BUTTON(always), GTK_CHECK_BUTTON(never));
  const int radio = DevicePropertiesLabels::RadioIndex(mode);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(never), radio == 0 ? TRUE : FALSE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(unsupported), radio == 1 ? TRUE : FALSE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(always), radio == 2 ? TRUE : FALSE);
  gtk_box_append(GTK_BOX(formats_box), never);
  gtk_box_append(GTK_BOX(formats_box), unsupported);
  gtk_box_append(GTK_BOX(formats_box), always);

  gtk_box_append(GTK_BOX(formats_box), gtk_label_new(Translations::CStr(DevicePropertiesLabels::PreferredFormat())));
  const auto formats = DevicePropertiesLabels::FormatChoices();
  std::vector<const char *> format_labels;
  format_labels.reserve(formats.size() + 1);
  for (const auto &choice : formats) {
    format_labels.push_back(choice.second.c_str());
  }
  format_labels.push_back(nullptr);
  GtkWidget *format = gtk_drop_down_new_from_strings(format_labels.data());
  gtk_box_append(GTK_BOX(formats_box), format);

  GtkWidget *not_connected = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(not_connected, 8);
  gtk_widget_set_margin_end(not_connected, 8);
  gtk_widget_set_margin_top(not_connected, 8);
  gtk_widget_set_margin_bottom(not_connected, 8);
  GtkWidget *not_open = gtk_label_new(Translations::CStr(DevicePropertiesLabels::FormatsNotConnected()));
  gtk_label_set_wrap(GTK_LABEL(not_open), TRUE);
  gtk_label_set_xalign(GTK_LABEL(not_open), 0.0f);
  gtk_box_append(GTK_BOX(not_connected), not_open);
  GtkWidget *open = gtk_button_new_with_label(Translations::CStr(DevicePropertiesLabels::OpenDevice()));
  gtk_widget_set_halign(open, GTK_ALIGN_START);
  gtk_widget_set_sensitive(open, DeviceSupportedFormats::OpenEnabled(device) ? TRUE : FALSE);
  auto *owned_open = new ConnectedDevice(device);
  g_object_set_data_full(G_OBJECT(open), "device", owned_open, [](gpointer p) { delete static_cast<ConnectedDevice *>(p); });
  g_signal_connect(open, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *device = static_cast<ConnectedDevice *>(g_object_get_data(G_OBJECT(button), "device"));
                     if (!application || !device || !application->device_manager()) {
                       return;
                     }
                     if (DeviceConnectDialog::NeedsMount(*device)) {
                       application->device_manager()->Mount(device->unique_id);
                       return;
                     }
                     if (device->mount_path.empty()) {
                       return;
                     }
                     GError *error = nullptr;
                     gchar *uri = g_filename_to_uri(device->mount_path.c_str(), nullptr, &error);
                     if (!uri) {
                       if (error) {
                         g_error_free(error);
                       }
                       return;
                     }
                     g_app_info_launch_default_for_uri(uri, nullptr, nullptr);
                     g_free(uri);
                   })),
                   app);
  gtk_box_append(GTK_BOX(not_connected), open);

  GtkWidget *loading = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_start(loading, 8);
  gtk_widget_set_margin_end(loading, 8);
  gtk_widget_set_margin_top(loading, 8);
  gtk_widget_set_margin_bottom(loading, 8);
  GtkWidget *spinner = gtk_spinner_new();
  gtk_spinner_start(GTK_SPINNER(spinner));
  gtk_box_append(GTK_BOX(loading), spinner);
  GtkWidget *querying = gtk_label_new(Translations::CStr(DevicePropertiesLabels::QueryingDevice()));
  gtk_label_set_xalign(GTK_LABEL(querying), 0.0f);
  gtk_widget_set_hexpand(querying, TRUE);
  gtk_box_append(GTK_BOX(loading), querying);

  GtkWidget *stack = gtk_stack_new();
  gtk_stack_add_named(GTK_STACK(stack), formats_box, DeviceSupportedFormats::StackName(DeviceSupportedFormats::Page::Formats));
  gtk_stack_add_named(GTK_STACK(stack), not_connected, DeviceSupportedFormats::StackName(DeviceSupportedFormats::Page::NotConnected));
  gtk_stack_add_named(GTK_STACK(stack), loading, DeviceSupportedFormats::StackName(DeviceSupportedFormats::Page::Loading));

  FormatsWidgets widgets;
  widgets.stack = stack;
  widgets.supported_box = supported_box;
  widgets.supported_list = supported_list;
  widgets.unsupported = unsupported;
  widgets.never = never;
  widgets.format = format;

  const DeviceSupportedFormats::Page page = DeviceSupportedFormats::PageFor(device);
  ShowFormatsPage(stack, page);
  if (page == DeviceSupportedFormats::Page::Formats) {
    ApplyFormats(widgets, DeviceSupportedFormats::Resolve(device.backend, {}, false, false), stored.id >= 0,
                 stored.id >= 0 ? stored.transcode_format : Song::FileType::Unknown);
  } else if (page == DeviceSupportedFormats::Page::Loading) {
    auto *job = new FormatsJob;
    job->alive = std::make_shared<bool>(true);
    job->widgets = widgets;
    job->serial = DeviceCopyJob::MtpSerial(device.unique_id);
    job->has_saved = stored.id >= 0;
    job->stored = stored.id >= 0 ? stored.transcode_format : Song::FileType::Unknown;
    auto *alive = new std::shared_ptr<bool>(job->alive);
    g_object_set_data_full(G_OBJECT(dialog), "formats-alive", alive, [](gpointer p) {
      **static_cast<std::shared_ptr<bool> *>(p) = false;
      delete static_cast<std::shared_ptr<bool> *>(p);
    });
    g_thread_unref(g_thread_new("device-formats", +[](gpointer data) -> gpointer {
      auto *job = static_cast<FormatsJob *>(data);
#ifdef HAVE_MTP
      MtpConnection connection;
      job->ok = connection.OpenBySerial(job->serial);
      if (job->ok) {
        job->types = connection.SupportedFiletypes();
        job->ok = !job->types.empty();
      }
#else
      job->ok = false;
#endif
      g_idle_add(
          +[](gpointer idle) -> gboolean {
            std::unique_ptr<FormatsJob> job(static_cast<FormatsJob *>(idle));
            if (!job->alive || !*job->alive) {
              return G_SOURCE_REMOVE;
            }
            ApplyFormats(job->widgets, DeviceSupportedFormats::Resolve("mtp", job->types, job->ok, true), job->has_saved, job->stored);
            ShowFormatsPage(job->widgets.stack, DeviceSupportedFormats::Page::Formats);
            return G_SOURCE_REMOVE;
          },
          job);
      return nullptr;
    }, job));
  }

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), stack, gtk_label_new(Translations::CStr(DevicePropertiesLabels::FileFormatsTab())));
  gtk_box_append(GTK_BOX(outer), notebook);

  GtkWidget *save = gtk_button_new_with_label(Translations::CStr(DevicePropertiesLabels::Save()));
  gtk_widget_add_css_class(save, "suggested-action");
  auto *owned = new ConnectedDevice(device);
  g_object_set_data_full(G_OBJECT(save), "device", owned, [](gpointer p) { delete static_cast<ConnectedDevice *>(p); });
  g_object_set_data(G_OBJECT(save), "name", name);
  g_object_set_data(G_OBJECT(save), "never", never);
  g_object_set_data(G_OBJECT(save), "unsupported", unsupported);
  g_object_set_data(G_OBJECT(save), "always", always);
  g_object_set_data(G_OBJECT(save), "format", format);
  g_object_set_data_full(G_OBJECT(save), "icon-name", selected_icon, [](gpointer p) { delete static_cast<std::string *>(p); });
  g_signal_connect(save, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *device = static_cast<ConnectedDevice *>(g_object_get_data(G_OBJECT(button), "device"));
                     if (!application || !device || !application->device_manager()) {
                       return;
                     }
                     const char *name = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "name")));
                     int radio = 1;
                     if (gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "never")))) {
                       radio = 0;
                     } else if (gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "always")))) {
                       radio = 2;
                     }
                     const guint format = gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "format")));
                     const auto *icon_name = static_cast<std::string *>(g_object_get_data(G_OBJECT(button), "icon-name"));
                     application->device_manager()->SetDeviceOptions(device->unique_id, name ? name : device->friendly_name,
                                                                     DevicePropertiesLabels::ModeFromRadio(radio),
                                                                     DevicePropertiesLabels::FormatAt(static_cast<int>(format)),
                                                                     icon_name ? *icon_name : device->icon);
                   }),
                   app);
  gtk_box_append(GTK_BOX(outer), save);
  adw_dialog_set_child(dialog, outer);
  adw_dialog_present(dialog, parent ? GTK_WIDGET(parent) : nullptr);
}
