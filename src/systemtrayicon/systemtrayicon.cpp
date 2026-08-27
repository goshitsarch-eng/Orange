#include "config.h"

#include "systemtrayicon/systemtrayicon.h"

#include "constants/behavioursettings.h"
#include "core/logging.h"
#include "core/settings.h"
#include "osd/osdprettyfade.h"
#include "systemtrayicon/trayiconcomposite.h"
#include "systemtrayicon/trayiconmask.h"
#include "systemtrayicon/trayiconpixmap.h"
#include "systemtrayicon/traymenulove.h"
#include "systemtrayicon/traymenumute.h"
#include "systemtrayicon/traymenustop.h"
#include "systemtrayicon/traymenuposition.h"
#include "systemtrayicon/traypopup.h"
#include "systemtrayicon/traysettingsreload.h"
#include "translations/translations.h"

#include <cairo.h>

#ifdef HAVE_X11
#include <gdk/x11/gdkx.h>
#endif

#include <algorithm>
#include <cstdint>
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
    "    <property name='IconPixmap' type='a(iiay)' access='read'/>"
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
    "    <signal name='NewOverlayIcon'/>"
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

constexpr char kMenuInterface[] = "com.canonical.dbusmenu";

const gchar kMenuXml[] =
    "<node>"
    "  <interface name='com.canonical.dbusmenu'>"
    "    <property name='Version' type='u' access='read'/>"
    "    <property name='TextDirection' type='s' access='read'/>"
    "    <property name='Status' type='s' access='read'/>"
    "    <property name='IconThemePath' type='as' access='read'/>"
    "    <method name='GetLayout'>"
    "      <arg type='i' name='parentId' direction='in'/>"
    "      <arg type='i' name='recursionDepth' direction='in'/>"
    "      <arg type='as' name='propertyNames' direction='in'/>"
    "      <arg type='u' name='revision' direction='out'/>"
    "      <arg type='(ia{sv}av)' name='layout' direction='out'/>"
    "    </method>"
    "    <method name='GetGroupProperties'>"
    "      <arg type='ai' name='ids' direction='in'/>"
    "      <arg type='as' name='propertyNames' direction='in'/>"
    "      <arg type='a(ia{sv})' name='properties' direction='out'/>"
    "    </method>"
    "    <method name='GetProperty'>"
    "      <arg type='i' name='id' direction='in'/>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='v' name='value' direction='out'/>"
    "    </method>"
    "    <method name='Event'>"
    "      <arg type='i' name='id' direction='in'/>"
    "      <arg type='s' name='eventId' direction='in'/>"
    "      <arg type='v' name='data' direction='in'/>"
    "      <arg type='u' name='timestamp' direction='in'/>"
    "    </method>"
    "    <method name='EventGroup'>"
    "      <arg type='a(isvu)' name='events' direction='in'/>"
    "      <arg type='ai' name='idErrors' direction='out'/>"
    "    </method>"
    "    <method name='AboutToShow'>"
    "      <arg type='i' name='id' direction='in'/>"
    "      <arg type='b' name='needUpdate' direction='out'/>"
    "    </method>"
    "    <method name='AboutToShowGroup'>"
    "      <arg type='ai' name='ids' direction='in'/>"
    "      <arg type='ai' name='updatesNeeded' direction='out'/>"
    "      <arg type='ai' name='idErrors' direction='out'/>"
    "    </method>"
    "    <signal name='ItemsPropertiesUpdated'>"
    "      <arg type='a(ia{sv})' name='updatedProps'/>"
    "      <arg type='a(ias)' name='removedProps'/>"
    "    </signal>"
    "    <signal name='LayoutUpdated'>"
    "      <arg type='u' name='revision'/>"
    "      <arg type='i' name='parent'/>"
    "    </signal>"
    "    <signal name='ItemActivationRequested'>"
    "      <arg type='i' name='id'/>"
    "      <arg type='u' name='timestamp'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

const GDBusInterfaceVTable kMenuVtable = {
    SystemTrayIcon::HandleMenuMethod,
    SystemTrayIcon::HandleMenuGetProperty,
    nullptr,
    {nullptr},
};

void SetPopupArt(GtkWidget *image, const std::vector<unsigned char> &data, int pixel_size) {
  if (!image || data.empty()) {
    return;
  }
  GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
  if (gdk_pixbuf_loader_write(loader, data.data(), data.size(), nullptr) && gdk_pixbuf_loader_close(loader, nullptr)) {
    GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    if (pixbuf) {
      GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, pixel_size, pixel_size, GDK_INTERP_BILINEAR);
      GdkTexture *texture = gdk_texture_new_for_pixbuf(scaled);
      gtk_image_set_from_paintable(GTK_IMAGE(image), GDK_PAINTABLE(texture));
      g_object_unref(texture);
      g_object_unref(scaled);
    }
  }
  g_object_unref(loader);
}

GVariant *ItemProps(const char *label, const char *type = "standard", bool enabled = true, bool visible = true, int toggle_state = -2) {
  GVariantBuilder props;
  g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&props, "{sv}", "type", g_variant_new_string(type));
  if (label && *label) {
    g_variant_builder_add(&props, "{sv}", "label", g_variant_new_string(label));
    g_variant_builder_add(&props, "{sv}", "enabled", g_variant_new_boolean(enabled));
    g_variant_builder_add(&props, "{sv}", "visible", g_variant_new_boolean(visible));
    if (toggle_state >= 0) {
      g_variant_builder_add(&props, "{sv}", "toggle-type", g_variant_new_string(TrayMenuMute::ToggleType()));
      g_variant_builder_add(&props, "{sv}", "toggle-state", g_variant_new_int32(toggle_state));
    }
  }
  return g_variant_builder_end(&props);
}

}  // namespace

std::vector<int> SystemTrayIcon::RootMenuIds(bool show_love, bool show_mute) {
  return TrayMenuMute::FilterMenuIds(TrayMenuLove::FilterMenuIds(AllMenuIds(), kMenuLove, show_love), kMenuMute, show_mute);
}

void SystemTrayIcon::SetLoveVisible(bool visible) {
  if (love_visible_ == visible) {
    return;
  }
  love_visible_ = visible;
  EmitLayoutUpdated();
}

void SystemTrayIcon::SetLoveEnabled(bool enabled) {
  if (love_enabled_ == enabled) {
    return;
  }
  love_enabled_ = enabled;
  EmitLayoutUpdated();
}

void SystemTrayIcon::SetMuteEnabled(bool enabled) {
  if (mute_enabled_ == enabled) {
    return;
  }
  mute_enabled_ = enabled;
  EmitLayoutUpdated();
}

void SystemTrayIcon::SetMuteChecked(bool checked) {
  if (mute_checked_ == checked) {
    return;
  }
  mute_checked_ = checked;
  EmitLayoutUpdated();
}

SystemTrayIcon::SystemTrayIcon() = default;

SystemTrayIcon::~SystemTrayIcon() {
  if (popup_timeout_id_) {
    g_source_remove(popup_timeout_id_);
    popup_timeout_id_ = 0;
  }
  if (popup_window_) {
    gtk_window_destroy(GTK_WINDOW(popup_window_));
    popup_window_ = nullptr;
  }
  TeardownStatusNotifier();
}

void SystemTrayIcon::ShowPopup(const std::string &summary, const std::string &message, int timeout_ms,
                               const std::vector<unsigned char> &art) {
  popup_summary_ = summary;
  popup_message_ = message;
  popup_timeout_ms_ = timeout_ms;
  popup_art_ = art;
  if (!gtk_is_initialized()) {
    return;
  }
  if (!popup_window_) {
    popup_window_ = gtk_window_new();
    gtk_window_set_decorated(GTK_WINDOW(popup_window_), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(popup_window_), FALSE);
    gtk_window_set_title(GTK_WINDOW(popup_window_), "Strawberry");
    gtk_widget_add_css_class(popup_window_, "osd");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    popup_image_ = gtk_image_new();
    gtk_widget_set_halign(popup_image_, GTK_ALIGN_START);
    popup_title_ = gtk_label_new("");
    gtk_widget_add_css_class(popup_title_, "title-4");
    gtk_label_set_xalign(GTK_LABEL(popup_title_), 0);
    popup_body_ = gtk_label_new("");
    gtk_widget_add_css_class(popup_body_, "dim-label");
    gtk_label_set_xalign(GTK_LABEL(popup_body_), 0);
    gtk_box_append(GTK_BOX(box), popup_image_);
    gtk_box_append(GTK_BOX(box), popup_title_);
    gtk_box_append(GTK_BOX(box), popup_body_);
    gtk_window_set_child(GTK_WINDOW(popup_window_), box);
  }
  gtk_label_set_text(GTK_LABEL(popup_title_), summary.c_str());
  gtk_label_set_text(GTK_LABEL(popup_body_), message.c_str());
  if (popup_image_) {
    if (TrayPopup::ShowArt(true, !art.empty())) {
      SetPopupArt(popup_image_, art, 64);
      gtk_widget_set_visible(popup_image_, TRUE);
    } else {
      gtk_widget_set_visible(popup_image_, FALSE);
    }
  }
  ++popup_fade_gen_;
  gtk_widget_set_opacity(popup_window_, 0.0);
  gtk_window_present(GTK_WINDOW(popup_window_));
  struct FadeJob {
    SystemTrayIcon *self = nullptr;
    int elapsed = 0;
    int gen = 0;
  };
  auto *job = new FadeJob{this, 0, popup_fade_gen_};
  g_timeout_add_full(
      G_PRIORITY_DEFAULT, static_cast<guint>(TrayPopup::FadeTickMs()),
      +[](gpointer data) -> gboolean {
        auto *fade = static_cast<FadeJob *>(data);
        if (!fade->self || fade->gen != fade->self->popup_fade_gen_ || !fade->self->popup_window_) {
          return G_SOURCE_REMOVE;
        }
        fade->elapsed += TrayPopup::FadeTickMs();
        gtk_widget_set_opacity(fade->self->popup_window_,
                               TrayPopup::FadeOpacity(fade->elapsed, TrayPopup::FadeDurationMs(), true));
        return OSDPrettyFade::Finished(fade->elapsed, TrayPopup::FadeDurationMs()) ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
      },
      job, +[](gpointer data) { delete static_cast<FadeJob *>(data); });
  if (popup_timeout_id_) {
    g_source_remove(popup_timeout_id_);
    popup_timeout_id_ = 0;
  }
  if (timeout_ms > 0) {
    popup_timeout_id_ = g_timeout_add(timeout_ms, [](gpointer data) -> gboolean {
      auto *self = static_cast<SystemTrayIcon *>(data);
      if (self->popup_window_) {
        gtk_widget_set_visible(self->popup_window_, FALSE);
      }
      self->popup_timeout_id_ = 0;
      return G_SOURCE_REMOVE;
    }, this);
  }
}

void SystemTrayIcon::RefreshPresentation() {
  UpdateTooltip();
  RebuildIconPixmap();
  EmitNewStatus();
  EmitNewOverlayIcon();
  EmitNewIcon();
  ++menu_revision_;
  EmitLayoutUpdated();
}

void SystemTrayIcon::SetPlaying(bool playing) {
  if (playing) {
    playing_ = true;
    paused_ = false;
  } else {
    SetStopped();
    return;
  }
  RefreshPresentation();
}

void SystemTrayIcon::SetPaused() {
  playing_ = false;
  paused_ = true;
  RefreshPresentation();
}

void SystemTrayIcon::SetStopped() {
  playing_ = false;
  paused_ = false;
  RefreshPresentation();
}

void SystemTrayIcon::SetProgress(int percentage) {
  progress_ = std::max(0, std::min(100, percentage));
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  if (settings.BoolValue(BehaviourSettings::kTrayIconProgress, BehaviourSettings::kDefaultTrayIconProgress)) {
    UpdateTooltip();
    RebuildIconPixmap();
    EmitNewToolTip();
    EmitNewOverlayIcon();
    EmitNewIcon();
  }
}

std::string SystemTrayIcon::OverlayIconName() const {
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  const bool enabled = settings.BoolValue(BehaviourSettings::kTrayIconProgress, BehaviourSettings::kDefaultTrayIconProgress);
  return TrayIconComposite::OverlayName(progress_, enabled, TrayIconComposite::StateFrom(playing_, paused_));
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

void SystemTrayIcon::EmitNewOverlayIcon() {
  if (!connection_ || registration_id_ == 0) {
    return;
  }
  g_dbus_connection_emit_signal(connection_, nullptr, kSniPath, kSniInterface, "NewOverlayIcon", nullptr, nullptr);
}

void SystemTrayIcon::EmitNewIcon() {
  if (!connection_ || registration_id_ == 0) {
    return;
  }
  g_dbus_connection_emit_signal(connection_, nullptr, kSniPath, kSniInterface, "NewIcon", nullptr, nullptr);
}

void SystemTrayIcon::RebuildIconPixmap() {
  icon_pixmap_.clear();
  icon_w_ = 0;
  icon_h_ = 0;
  if (!gtk_is_initialized()) {
    return;
  }
  const int size = TrayIconPixmap::kDefaultSize;
  cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
  cairo_t *cr = cairo_create(surface);
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_paint(cr);
  cairo_set_source_rgb(cr, 0.77, 0.36, 0.15);
  cairo_arc(cr, size / 2.0, size / 2.0, size * 0.38, 0, 6.283185307179586);
  cairo_fill(cr);
  cairo_set_source_rgb(cr, 0.45, 0.62, 0.22);
  cairo_arc(cr, size * 0.62, size * 0.28, size * 0.10, 0, 6.283185307179586);
  cairo_fill(cr);

  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  const bool progress_enabled = settings.BoolValue(BehaviourSettings::kTrayIconProgress, BehaviourSettings::kDefaultTrayIconProgress);
  const TrayIconComposite::Playback state = TrayIconComposite::StateFrom(playing_, paused_);
  if (TrayIconComposite::ShowsProgress(progress_, progress_enabled, state)) {
    cairo_save(cr);
    const auto mask = TrayIconMask::ProgressMask(size, size, progress_);
    if (mask.size() >= 3) {
      cairo_move_to(cr, mask[0].x, mask[0].y);
      for (size_t i = 1; i < mask.size(); ++i) {
        cairo_line_to(cr, mask[i].x, mask[i].y);
      }
      cairo_close_path(cr);
      cairo_clip(cr);
      cairo_set_source_rgba(cr, 0.55, 0.55, 0.55, 0.78);
      cairo_paint(cr);
    }
    cairo_restore(cr);
  }
  if (state != TrayIconComposite::Playback::Stopped) {
    const int badge_h = TrayIconComposite::BadgeHeight(size);
    const TrayIconComposite::Point origin = TrayIconComposite::BadgeTopLeft(size, badge_h);
    cairo_save(cr);
    cairo_translate(cr, origin.x, origin.y);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.45);
    cairo_rectangle(cr, 0, 0, badge_h, badge_h);
    cairo_fill(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    if (state == TrayIconComposite::Playback::Playing) {
      cairo_move_to(cr, badge_h * 0.28, badge_h * 0.2);
      cairo_line_to(cr, badge_h * 0.28, badge_h * 0.8);
      cairo_line_to(cr, badge_h * 0.82, badge_h * 0.5);
      cairo_close_path(cr);
      cairo_fill(cr);
    } else {
      cairo_rectangle(cr, badge_h * 0.22, badge_h * 0.2, badge_h * 0.2, badge_h * 0.6);
      cairo_rectangle(cr, badge_h * 0.58, badge_h * 0.2, badge_h * 0.2, badge_h * 0.6);
      cairo_fill(cr);
    }
    cairo_restore(cr);
  }
  cairo_surface_flush(surface);
  const unsigned char *data = cairo_image_surface_get_data(surface);
  const int stride = cairo_image_surface_get_stride(surface);
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS || !data) {
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return;
  }
  icon_w_ = size;
  icon_h_ = size;
  icon_pixmap_.reserve(TrayIconPixmap::ByteCount(size, size));
  for (int y = 0; y < size; ++y) {
    const auto *row = reinterpret_cast<const uint32_t *>(data + y * stride);
    TrayIconPixmap::PackNativeArgbRow(row, size, &icon_pixmap_);
  }
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

void SystemTrayIcon::PositionMenuWindow(GtkWidget *window, int x, int y) {
  if (!window || !TrayMenuPosition::HasScreenPoint(x, y)) {
    return;
  }
  gtk_widget_realize(window);
#ifdef HAVE_X11
  GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
  if (surface && GDK_IS_X11_SURFACE(surface)) {
    const TrayMenuPosition::Rect anchor = TrayMenuPosition::AnchorPoint(x, y);
    Display *display = gdk_x11_display_get_xdisplay(gdk_surface_get_display(surface));
    XMoveWindow(display, gdk_x11_surface_get_xid(GDK_X11_SURFACE(surface)), anchor.x, anchor.y);
  }
#else
  (void)x;
  (void)y;
#endif
}

void SystemTrayIcon::ShowMenu(int x, int y) {
  last_menu_x_ = x;
  last_menu_y_ = y;
  if (!gtk_is_initialized()) {
    return;
  }
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
  connect(box, gtk_button_new_with_label(playing_ ? Translations::CStr("Pause") : Translations::CStr("Play")), &PlayPause);
  {
    GtkWidget *stop = gtk_button_new_with_label(Translations::CStr("Stop"));
    gtk_widget_set_sensitive(stop, TrayMenuStop::PlaybackActive(playing_, paused_) ? TRUE : FALSE);
    connect(box, stop, &Stop);
  }
  connect(box, gtk_button_new_with_label(Translations::CStr("Next")), &Next);
  connect(box, gtk_button_new_with_label(Translations::CStr("Previous")), &Previous);
  if (mute_enabled_) {
    GtkWidget *mute = gtk_check_button_new_with_label(Translations::CStr("Mute"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(mute), mute_checked_ ? TRUE : FALSE);
    gtk_widget_add_css_class(mute, "flat");
    g_signal_connect(mute, "toggled",
                     G_CALLBACK((+[](GtkCheckButton *btn, gpointer data) {
                       static_cast<Signal<> *>(data)->Emit();
                       if (GtkWidget *win = gtk_widget_get_ancestor(GTK_WIDGET(btn), GTK_TYPE_WINDOW)) {
                         gtk_window_destroy(GTK_WINDOW(win));
                       }
                     })),
                     &Mute);
    gtk_box_append(GTK_BOX(box), mute);
  }
  {
    GtkWidget *stop_after = gtk_button_new_with_label(Translations::CStr("Stop after this track"));
    gtk_widget_set_sensitive(stop_after, TrayMenuStop::PlaybackActive(playing_, paused_) ? TRUE : FALSE);
    connect(box, stop_after, &StopAfter);
  }
  if (love_visible_) {
    GtkWidget *love = gtk_button_new_with_label(Translations::CStr("Love"));
    gtk_widget_set_sensitive(love, love_enabled_ ? TRUE : FALSE);
    connect(box, love, &Love);
  }
  connect(box, gtk_button_new_with_label(Translations::CStr("Show / Hide")), &ShowHide);
  connect(box, gtk_button_new_with_label(Translations::CStr("Quit")), &Quit);
  gtk_window_set_child(GTK_WINDOW(window), box);
  gtk_window_present(GTK_WINDOW(window));
  PositionMenuWindow(window, x, y);
}

const char *SystemTrayIcon::MenuLabel(int id, bool playing) {
  switch (id) {
    case kMenuPlayPause:
      return playing ? Translations::CStr("Pause") : Translations::CStr("Play");
    case kMenuStop:
      return Translations::CStr("Stop");
    case kMenuNext:
      return Translations::CStr("Next");
    case kMenuPrevious:
      return Translations::CStr("Previous");
    case kMenuShowHide:
      return Translations::CStr("Show / Hide");
    case kMenuQuit:
      return Translations::CStr("Quit");
    case kMenuMute:
      return Translations::CStr("Mute");
    case kMenuStopAfter:
      return Translations::CStr("Stop after this track");
    case kMenuLove:
      return Translations::CStr("Love");
    default:
      return "";
  }
}

bool SystemTrayIcon::ActivateMenuId(int id, Signal<> *play_pause, Signal<> *stop, Signal<> *next, Signal<> *previous,
                                   Signal<> *show_hide, Signal<> *quit, Signal<> *mute, Signal<> *stop_after, Signal<> *love) {
  switch (id) {
    case kMenuPlayPause:
      if (play_pause) {
        play_pause->Emit();
      }
      return true;
    case kMenuStop:
      if (stop) {
        stop->Emit();
      }
      return true;
    case kMenuNext:
      if (next) {
        next->Emit();
      }
      return true;
    case kMenuPrevious:
      if (previous) {
        previous->Emit();
      }
      return true;
    case kMenuShowHide:
      if (show_hide) {
        show_hide->Emit();
      }
      return true;
    case kMenuQuit:
      if (quit) {
        quit->Emit();
      }
      return true;
    case kMenuMute:
      if (mute) {
        mute->Emit();
      }
      return true;
    case kMenuStopAfter:
      if (stop_after) {
        stop_after->Emit();
      }
      return true;
    case kMenuLove:
      if (love) {
        love->Emit();
      }
      return true;
    default:
      return false;
  }
}

GVariant *SystemTrayIcon::MenuLayout(int parent_id) const {
  if (parent_id != 0) {
    GVariantBuilder props;
    g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
    GVariantBuilder children;
    g_variant_builder_init(&children, G_VARIANT_TYPE("av"));
    return g_variant_new("(ia{sv}av)", parent_id, &props, &children);
  }
  GVariantBuilder root_props;
  g_variant_builder_init(&root_props, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&root_props, "{sv}", "children-display", g_variant_new_string("submenu"));
  GVariantBuilder children;
  g_variant_builder_init(&children, G_VARIANT_TYPE("av"));
  const std::vector<int> ids = RootMenuIds(love_visible_, mute_enabled_);
  for (int id : ids) {
    GVariantBuilder empty;
    g_variant_builder_init(&empty, G_VARIANT_TYPE("av"));
    const char *type = IsSeparatorId(id) ? "separator" : "standard";
    const bool enabled = TrayMenuLove::ItemEnabled(id, kMenuLove, love_enabled_) &&
                         TrayMenuStop::ItemEnabled(id, kMenuStop, kMenuStopAfter, TrayMenuStop::PlaybackActive(playing_, paused_));
    const bool visible = TrayMenuLove::ItemVisible(id, kMenuLove, love_visible_) && TrayMenuMute::ItemVisible(id, kMenuMute, mute_enabled_);
    const int toggle_state = TrayMenuMute::ToggleStateForId(id, kMenuMute, mute_checked_);
    GVariant *item = g_variant_new("(i@a{sv}av)", id, ItemProps(MenuLabel(id, playing_), type, enabled, visible, toggle_state), &empty);
    g_variant_builder_add(&children, "v", item);
  }
  return g_variant_new("(ia{sv}av)", 0, &root_props, &children);
}

void SystemTrayIcon::EmitLayoutUpdated() {
  if (!connection_ || menu_registration_id_ == 0) {
    return;
  }
  g_dbus_connection_emit_signal(connection_, nullptr, kMenuObjectPath, kMenuInterface, "LayoutUpdated",
                                g_variant_new("(ui)", menu_revision_, 0), nullptr);
}

void SystemTrayIcon::RegisterMenu(GDBusConnection *connection) {
  GError *error = nullptr;
  GDBusNodeInfo *info = g_dbus_node_info_new_for_xml(kMenuXml, &error);
  if (!info) {
    if (error) {
      LogError("DBusMenu XML: %s", error->message);
      g_error_free(error);
    }
    return;
  }
  menu_registration_id_ =
      g_dbus_connection_register_object(connection, kMenuObjectPath, info->interfaces[0], &kMenuVtable, this, nullptr, &error);
  g_dbus_node_info_unref(info);
  if (error) {
    LogError("DBusMenu register: %s", error->message);
    g_error_free(error);
    return;
  }
  menu_path_ = kMenuObjectPath;
}

void SystemTrayIcon::TeardownStatusNotifier() {
  if (connection_ && menu_registration_id_ != 0) {
    g_dbus_connection_unregister_object(connection_, menu_registration_id_);
    menu_registration_id_ = 0;
  }
  if (connection_ && registration_id_ != 0) {
    g_dbus_connection_unregister_object(connection_, registration_id_);
    registration_id_ = 0;
  }
  if (owner_id_ != 0) {
    g_bus_unown_name(owner_id_);
    owner_id_ = 0;
  }
  connection_ = nullptr;
  available_ = false;
  visible_ = false;
  menu_path_ = "/NO_DBUSMENU";
}

void SystemTrayIcon::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  const bool show = TraySettingsReload::ShowTray(settings.BoolValue(BehaviourSettings::kShowTrayIcon, BehaviourSettings::kDefaultShowTrayIcon));
  const bool registered = TraySettingsReload::IsRegistered(owner_id_);
  if (TraySettingsReload::ShouldUnregister(show, registered)) {
    TeardownStatusNotifier();
    return;
  }
  if (TraySettingsReload::ShouldRegister(show, registered)) {
    SetupStatusNotifier();
    return;
  }
  if (show && TraySettingsReload::ShouldRefreshProgress()) {
    SetVisible(true);
    RefreshPresentation();
  }
}

void SystemTrayIcon::SetupStatusNotifier() {
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  if (!TraySettingsReload::ShowTray(settings.BoolValue(BehaviourSettings::kShowTrayIcon, BehaviourSettings::kDefaultShowTrayIcon))) {
    available_ = false;
    visible_ = false;
    return;
  }
  if (TraySettingsReload::IsRegistered(owner_id_)) {
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
  self->RegisterMenu(connection);
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
    return g_variant_new_string(TrayIconComposite::BaseIconName());
  }
  if (g_strcmp0(property_name, "IconPixmap") == 0) {
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a(iiay)"));
    if (!self->icon_pixmap_.empty() && self->icon_w_ > 0 && self->icon_h_ > 0) {
      GVariant *bytes = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, self->icon_pixmap_.data(), self->icon_pixmap_.size(), 1);
      g_variant_builder_add(&builder, "(ii@ay)", self->icon_w_, self->icon_h_, bytes);
    }
    return g_variant_builder_end(&builder);
  }
  if (g_strcmp0(property_name, "OverlayIconName") == 0) {
    const std::string overlay = self->OverlayIconName();
    return g_variant_new_string(overlay.c_str());
  }
  if (g_strcmp0(property_name, "AttentionIconName") == 0) {
    return g_variant_new_string("");
  }
  if (g_strcmp0(property_name, "ItemIsMenu") == 0) {
    return g_variant_new_boolean(self->menu_path_ == kMenuObjectPath);
  }
  if (g_strcmp0(property_name, "Menu") == 0) {
    return g_variant_new_object_path(self->menu_path_.c_str());
  }
  if (g_strcmp0(property_name, "ToolTip") == 0) {
    return g_variant_new_parsed("(%s, @a(iiay) [], %s, %s)", "", "Strawberry", self->tooltip_.c_str());
  }
  g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Unknown property %s", property_name);
  return nullptr;
}

void SystemTrayIcon::HandleMenuMethod(GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *method_name,
                                      GVariant *parameters, GDBusMethodInvocation *invocation, gpointer data) {
  auto *self = static_cast<SystemTrayIcon *>(data);
  if (g_strcmp0(method_name, "GetLayout") == 0) {
    gint parent = 0;
    gint depth = 0;
    g_variant_get(parameters, "(iias)", &parent, &depth, nullptr);
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(u@(ia{sv}av))", self->menu_revision_, self->MenuLayout(parent)));
    return;
  }
  if (g_strcmp0(method_name, "GetGroupProperties") == 0) {
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a(ia{sv})"));
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(a(ia{sv}))", &builder));
    return;
  }
  if (g_strcmp0(method_name, "GetProperty") == 0) {
    gint id = 0;
    const gchar *name = nullptr;
    g_variant_get(parameters, "(i&s)", &id, &name);
    GVariant *value = g_variant_new_string(MenuLabel(id, self->playing_));
    if (g_strcmp0(name, "type") == 0) {
      value = g_variant_new_string(IsSeparatorId(id) ? "separator" : "standard");
    } else if (g_strcmp0(name, "enabled") == 0) {
      value = g_variant_new_boolean(TrayMenuLove::ItemEnabled(id, kMenuLove, self->love_enabled_));
    } else if (g_strcmp0(name, "visible") == 0) {
      value = g_variant_new_boolean(TrayMenuLove::ItemVisible(id, kMenuLove, self->love_visible_));
    }
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(v)", value));
    return;
  }
  if (g_strcmp0(method_name, "Event") == 0) {
    gint id = 0;
    const gchar *event_id = nullptr;
    g_variant_get(parameters, "(i&svu)", &id, &event_id, nullptr, nullptr);
    if (g_strcmp0(event_id, "clicked") == 0) {
      ActivateMenuId(id, &self->PlayPause, &self->Stop, &self->Next, &self->Previous, &self->ShowHide, &self->Quit, &self->Mute,
                     &self->StopAfter, &self->Love);
    }
    g_dbus_method_invocation_return_value(invocation, nullptr);
    return;
  }
  if (g_strcmp0(method_name, "EventGroup") == 0) {
    GVariantIter *iter = nullptr;
    g_variant_get(parameters, "(a(isvu))", &iter);
    gint id = 0;
    const gchar *event_id = nullptr;
    while (g_variant_iter_next(iter, "(i&svu)", &id, &event_id, nullptr, nullptr)) {
      if (g_strcmp0(event_id, "clicked") == 0) {
        ActivateMenuId(id, &self->PlayPause, &self->Stop, &self->Next, &self->Previous, &self->ShowHide, &self->Quit, &self->Mute,
                     &self->StopAfter, &self->Love);
      }
    }
    g_variant_iter_free(iter);
    GVariantBuilder errors;
    g_variant_builder_init(&errors, G_VARIANT_TYPE("ai"));
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(ai)", &errors));
    return;
  }
  if (g_strcmp0(method_name, "AboutToShow") == 0) {
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
    return;
  }
  if (g_strcmp0(method_name, "AboutToShowGroup") == 0) {
    GVariantBuilder needed;
    GVariantBuilder errors;
    g_variant_builder_init(&needed, G_VARIANT_TYPE("ai"));
    g_variant_builder_init(&errors, G_VARIANT_TYPE("ai"));
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(aiai)", &needed, &errors));
    return;
  }
  g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown method %s", method_name);
}

GVariant *SystemTrayIcon::HandleMenuGetProperty(GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *property_name,
                                                GError **error, gpointer) {
  if (g_strcmp0(property_name, "Version") == 0) {
    return g_variant_new_uint32(3);
  }
  if (g_strcmp0(property_name, "TextDirection") == 0) {
    return g_variant_new_string("ltr");
  }
  if (g_strcmp0(property_name, "Status") == 0) {
    return g_variant_new_string("normal");
  }
  if (g_strcmp0(property_name, "IconThemePath") == 0) {
    return g_variant_new_strv(nullptr, 0);
  }
  g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Unknown property %s", property_name);
  return nullptr;
}
