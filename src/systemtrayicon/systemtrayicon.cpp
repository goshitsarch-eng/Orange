#include "systemtrayicon/systemtrayicon.h"

#include "core/logging.h"
#include "core/settings.h"
#include "constants/behavioursettings.h"

#include <algorithm>
#include <unistd.h>

namespace {

constexpr char kSniInterface[] = "org.kde.StatusNotifierItem";
constexpr char kSniPath[] = "/StatusNotifierItem";

const gchar kSniXml[] =
    "<node>"
    "  <interface name='org.kde.StatusNotifierItem'>"
    "    <property name='Category' type='s' access='read'/>"
    "    <property name='Id' type='s' access='read'/>"
    "    <property name='Title' type='s' access='read'/>"
    "    <property name='Status' type='s' access='read'/>"
    "    <property name='WindowId' type='i' access='read'/>"
    "    <property name='IconName' type='s' access='read'/>"
    "    <property name='OverlayIconName' type='s' access='read'/>"
    "    <property name='AttentionIconName' type='s' access='read'/>"
    "    <property name='ToolTip' type='(sa(iiay)ss)' access='read'/>"
    "    <property name='ItemIsMenu' type='b' access='read'/>"
    "    <property name='Menu' type='o' access='read'/>"
    "    <method name='ContextMenu'>"
    "      <arg type='i' name='x' direction='in'/>"
    "      <arg type='i' name='y' direction='in'/>"
    "    </method>"
    "    <method name='Activate'>"
    "      <arg type='i' name='x' direction='in'/>"
    "      <arg type='i' name='y' direction='in'/>"
    "    </method>"
    "    <method name='SecondaryActivate'>"
    "      <arg type='i' name='x' direction='in'/>"
    "      <arg type='i' name='y' direction='in'/>"
    "    </method>"
    "    <method name='Scroll'>"
    "      <arg type='i' name='delta' direction='in'/>"
    "      <arg type='s' name='orientation' direction='in'/>"
    "    </method>"
    "    <signal name='NewTitle'/>"
    "    <signal name='NewIcon'/>"
    "    <signal name='NewToolTip'/>"
    "    <signal name='NewStatus'><arg type='s' name='status'/></signal>"
    "  </interface>"
    "</node>";

const GDBusInterfaceVTable kVtable = {
    SystemTrayIcon::HandleMethod,
    SystemTrayIcon::HandleGetProperty,
    nullptr,
    {nullptr},
};

}  // namespace

SystemTrayIcon::SystemTrayIcon() = default;

SystemTrayIcon::~SystemTrayIcon() {
  if (owner_id_ != 0) {
    g_bus_unown_name(owner_id_);
  }
}

void SystemTrayIcon::SetPlaying(bool playing) {
  playing_ = playing;
  UpdateTooltip();
  EmitNewStatus();
}

void SystemTrayIcon::SetProgress(int percentage) {
  progress_ = std::max(0, std::min(100, percentage));
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  if (settings.BoolValue(BehaviourSettings::kTrayIconProgress, BehaviourSettings::kDefaultTrayIconProgress)) {
    UpdateTooltip();
    EmitNewToolTip();
  }
}

void SystemTrayIcon::SetNowPlaying(const Song &song) {
  song_ = song;
  UpdateTooltip();
  EmitNewToolTip();
}

void SystemTrayIcon::ClearNowPlaying() {
  song_ = Song();
  UpdateTooltip();
  EmitNewToolTip();
}

void SystemTrayIcon::SetVisible(bool visible) {
  visible_ = visible;
  EmitNewStatus();
}

void SystemTrayIcon::UpdateTooltip() {
  if (!song_.is_valid() && song_.title().empty()) {
    tooltip_ = "Strawberry";
  } else {
    tooltip_ = song_.PrettyTitleWithArtist();
    if (playing_ && progress_ > 0) {
      tooltip_ += " (" + std::to_string(progress_) + "%)";
    }
  }
}

void SystemTrayIcon::EmitNewToolTip() {
  if (!connection_ || registration_id_ == 0) {
    return;
  }
  g_dbus_connection_emit_signal(connection_, nullptr, kSniPath, kSniInterface, "NewToolTip", nullptr, nullptr);
}

void SystemTrayIcon::EmitNewStatus() {
  if (!connection_ || registration_id_ == 0) {
    return;
  }
  g_dbus_connection_emit_signal(connection_, nullptr, kSniPath, kSniInterface, "NewStatus",
                                g_variant_new("(s)", visible_ ? "Active" : "Passive"), nullptr);
}

void SystemTrayIcon::ShowMenu(int, int) {
  GtkWidget *window = gtk_window_new();
  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_widget_add_css_class(window, "osd");
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  auto connect = [](GtkWidget *box, GtkWidget *button, Signal<> *signal) {
    gtk_widget_add_css_class(button, "flat");
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       static_cast<Signal<> *>(data)->Emit();
                       if (GtkWidget *win = gtk_widget_get_ancestor(GTK_WIDGET(btn), GTK_TYPE_WINDOW)) {
                         gtk_window_destroy(GTK_WINDOW(win));
                       }
                     }),
                     signal);
    gtk_box_append(GTK_BOX(box), button);
  };
  connect(box, gtk_button_new_with_label(playing_ ? "Pause" : "Play"), &PlayPause);
  connect(box, gtk_button_new_with_label("Stop"), &Stop);
  connect(box, gtk_button_new_with_label("Next"), &Next);
  connect(box, gtk_button_new_with_label("Previous"), &Previous);
  connect(box, gtk_button_new_with_label("Show / Hide"), &ShowHide);
  connect(box, gtk_button_new_with_label("Quit"), &Quit);
  gtk_window_set_child(GTK_WINDOW(window), box);
  gtk_window_present(GTK_WINDOW(window));
}

void SystemTrayIcon::SetupStatusNotifier() {
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  if (!settings.BoolValue(BehaviourSettings::kShowTrayIcon, BehaviourSettings::kDefaultShowTrayIcon)) {
    available_ = false;
    visible_ = false;
    return;
  }
  service_name_ = "org.kde.StatusNotifierItem-" + std::to_string(getpid()) + "-1";
  owner_id_ = g_bus_own_name(G_BUS_TYPE_SESSION, service_name_.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE, OnBusAcquired, nullptr, OnNameLost,
                             this, nullptr);
}

void SystemTrayIcon::OnBusAcquired(GDBusConnection *connection, const gchar *, gpointer data) {
  auto *self = static_cast<SystemTrayIcon *>(data);
  self->connection_ = connection;
  GError *error = nullptr;
  GDBusNodeInfo *info = g_dbus_node_info_new_for_xml(kSniXml, &error);
  if (!info) {
    if (error) {
      LogError("StatusNotifier XML: %s", error->message);
      g_error_free(error);
    }
    return;
  }
  self->registration_id_ =
      g_dbus_connection_register_object(connection, kSniPath, info->interfaces[0], &kVtable, self, nullptr, &error);
  g_dbus_node_info_unref(info);
  if (error) {
    LogError("StatusNotifier register: %s", error->message);
    g_error_free(error);
    return;
  }
  self->available_ = true;
  self->visible_ = true;
  g_dbus_connection_call(connection, "org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher",
                         "RegisterStatusNotifierItem", g_variant_new("(s)", self->service_name_.c_str()), nullptr,
                         G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
}

void SystemTrayIcon::OnNameLost(GDBusConnection *, const gchar *, gpointer data) {
  auto *self = static_cast<SystemTrayIcon *>(data);
  self->available_ = false;
  self->visible_ = false;
}

void SystemTrayIcon::HandleMethod(GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *method_name,
                                 GVariant *parameters, GDBusMethodInvocation *invocation, gpointer data) {
  auto *self = static_cast<SystemTrayIcon *>(data);
  if (g_strcmp0(method_name, "Activate") == 0 || g_strcmp0(method_name, "SecondaryActivate") == 0) {
    self->ShowHide.Emit();
  } else if (g_strcmp0(method_name, "ContextMenu") == 0) {
    gint x = 0;
    gint y = 0;
    g_variant_get(parameters, "(ii)", &x, &y);
    self->ShowMenu(x, y);
  } else if (g_strcmp0(method_name, "Scroll") == 0) {
    gint delta = 0;
    const gchar *orientation = nullptr;
    g_variant_get(parameters, "(i&s)", &delta, &orientation);
    if (orientation && g_ascii_strcasecmp(orientation, "vertical") == 0) {
      self->VolumeScroll.Emit(delta);
    }
  }
  g_dbus_method_invocation_return_value(invocation, nullptr);
}

GVariant *SystemTrayIcon::HandleGetProperty(GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *property_name,
                                            GError **error, gpointer data) {
  auto *self = static_cast<SystemTrayIcon *>(data);
  if (g_strcmp0(property_name, "Category") == 0) {
    return g_variant_new_string("ApplicationStatus");
  }
  if (g_strcmp0(property_name, "Id") == 0) {
    return g_variant_new_string("strawberry");
  }
  if (g_strcmp0(property_name, "Title") == 0) {
    return g_variant_new_string("Strawberry");
  }
  if (g_strcmp0(property_name, "Status") == 0) {
    return g_variant_new_string(self->visible_ ? "Active" : "Passive");
  }
  if (g_strcmp0(property_name, "WindowId") == 0) {
    return g_variant_new_int32(0);
  }
  if (g_strcmp0(property_name, "IconName") == 0) {
    return g_variant_new_string(self->playing_ ? "media-playback-start" : "strawberry");
  }
  if (g_strcmp0(property_name, "OverlayIconName") == 0 || g_strcmp0(property_name, "AttentionIconName") == 0) {
    return g_variant_new_string("");
  }
  if (g_strcmp0(property_name, "ItemIsMenu") == 0) {
    return g_variant_new_boolean(FALSE);
  }
  if (g_strcmp0(property_name, "Menu") == 0) {
    return g_variant_new_object_path("/NO_DBUSMENU");
  }
  if (g_strcmp0(property_name, "ToolTip") == 0) {
    return g_variant_new_parsed("(%s, @a(iiay) [], %s, %s)", "", "Strawberry", self->tooltip_.c_str());
  }
  g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Unknown property %s", property_name);
  return nullptr;
}
