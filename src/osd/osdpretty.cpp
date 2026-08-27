#include "osd/osdpretty.h"

#include "config.h"
#include "constants/notificationssettings.h"
#include "core/settings.h"
#include "osd/osdprettyfade.h"
#include "osd/osdprettylimits.h"
#include "osd/osdprettyplacement.h"
#include "osd/osdprettypopup.h"
#include "osd/osdprettytransparency.h"
#include "osd/osdprettywayland.h"
#include "utilities/fontutils.h"

#include <algorithm>
#include <utility>

#ifdef HAVE_X11
#include <gdk/x11/gdkx.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#endif

namespace {

struct ListedMonitor {
  std::string name;
  OSDPrettyPlacement::Rect workarea;
};

std::string MonitorName(GdkMonitor *monitor, int index) {
  if (!monitor) {
    return std::to_string(index);
  }
  if (const char *connector = gdk_monitor_get_connector(monitor); connector && *connector) {
    return connector;
  }
#if GTK_CHECK_VERSION(4, 10, 0)
  if (const char *description = gdk_monitor_get_description(monitor); description && *description) {
    return description;
  }
#endif
  if (const char *model = gdk_monitor_get_model(monitor); model && *model) {
    return model;
  }
  return std::to_string(index);
}

std::vector<ListedMonitor> ListMonitors() {
  std::vector<ListedMonitor> out;
  GdkDisplay *display = gdk_display_get_default();
  if (!display) {
    return out;
  }
  GListModel *model = gdk_display_get_monitors(display);
  const guint n = g_list_model_get_n_items(model);
  for (guint i = 0; i < n; ++i) {
    auto *monitor = GDK_MONITOR(g_list_model_get_item(model, i));
    ListedMonitor item;
    item.name = MonitorName(monitor, static_cast<int>(i));
    GdkRectangle geo{};
    gdk_monitor_get_geometry(monitor, &geo);
#ifdef HAVE_X11
    if (GDK_IS_X11_MONITOR(monitor)) {
      gdk_x11_monitor_get_workarea(monitor, &geo);
    }
#endif
    item.workarea = {geo.x, geo.y, geo.width, geo.height};
    out.push_back(item);
    g_object_unref(monitor);
  }
  return out;
}

OSDPrettyPlacement::Rect WindowSize(GtkWidget *window) {
  int width = 320;
  int height = 80;
  if (window) {
    GtkRequisition nat{};
    gtk_widget_get_preferred_size(window, nullptr, &nat);
    width = std::max({nat.width, gtk_widget_get_width(window), 160});
    height = std::max({nat.height, gtk_widget_get_height(window), 48});
  }
  return {0, 0, width, height};
}

void MoveWindow(GtkWidget *window, int x, int y) {
  bool is_x11 = false;
#ifdef HAVE_X11
  if (window) {
    if (GdkDisplay *display = gtk_widget_get_display(window)) {
      is_x11 = GDK_IS_X11_DISPLAY(display);
    }
  }
#endif
  if (!OSDPrettyWayland::CanMoveWindow(OSDPrettyWayland::DetectBackend(is_x11, false))) {
    (void)window;
    (void)x;
    (void)y;
    return;
  }
#ifdef HAVE_X11
  GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
  if (surface && GDK_IS_X11_SURFACE(surface)) {
    XMoveWindow(GDK_SURFACE_XDISPLAY(surface), gdk_x11_surface_get_xid(surface), x, y);
  }
#else
  (void)window;
  (void)x;
  (void)y;
#endif
}

OSDPrettyPlacement::Rect CurrentWorkarea(const std::string &screen) {
  const auto monitors = ListMonitors();
  std::vector<std::string> names;
  names.reserve(monitors.size());
  for (const auto &monitor : monitors) {
    names.push_back(monitor.name);
  }
  const int index = OSDPrettyPlacement::ResolveIndex(screen, names);
  if (index >= 0 && static_cast<size_t>(index) < monitors.size()) {
    return monitors[static_cast<size_t>(index)].workarea;
  }
  return {0, 0, 1920, 1080};
}

}  // namespace

OSDPretty::OSDPretty(Mode mode) : mode_(mode) { ReloadSettings(); }

OSDPretty::~OSDPretty() {
  StopFade();
  if (timeout_id_) {
    g_source_remove(timeout_id_);
  }
  if (window_) {
    gtk_window_destroy(GTK_WINDOW(window_));
  }
}

void OSDPretty::ReloadSettings() {
  Settings s;
  s.BeginGroup(OSDPrettySettings::kSettingsGroup);
  fg_ = s.Contains(OSDPrettySettings::kForegroundColor) ? s.Value(OSDPrettySettings::kForegroundColor)
                                                        : s.Value("foreground", "#ffffff");
  bg_ = s.Contains(OSDPrettySettings::kBackgroundColor) ? s.Value(OSDPrettySettings::kBackgroundColor)
                                                        : s.Value("background", "#202020");
  opacity_ = s.Contains(OSDPrettySettings::kBackgroundOpacity) ? s.DoubleValue(OSDPrettySettings::kBackgroundOpacity, 0.85)
                                                              : s.DoubleValue("opacity", 0.92);
  font_ = s.Contains(OSDPrettySettings::kFont) ? s.Value(OSDPrettySettings::kFont, OSDPrettySettings::kDefaultFont)
                                               : s.Value("font", "Sans 12");
  s.BeginGroup(OSDSettings::kSettingsGroup);
  pos_x_ = s.IntValue("posx", 40);
  pos_y_ = s.IntValue("posy", 40);
  show_art_ = s.Contains(OSDSettings::kShowArt) ? s.BoolValue(OSDSettings::kShowArt, true) : s.BoolValue("showart", true);
  timeout_ms_ = s.Contains(OSDSettings::kTimeout) ? s.IntValue(OSDSettings::kTimeout, 5000) : s.IntValue("timeout", 4000);
  s.BeginGroup(OSDPrettySettings::kSettingsGroup);
  fading_ = s.BoolValue(OSDPrettySettings::kFading, true);
  disable_duration_ = s.BoolValue(OSDPrettySettings::kDisableDuration, OSDPrettySettings::kDefaultDisableDuration);
  popup_screen_ = s.Value(OSDPrettySettings::kPopupScreen);
  if (s.Contains(OSDPrettySettings::kPopupPos)) {
    const OSDPrettyPlacement::Point pos = OSDPrettyPlacement::ParsePos(s.Value(OSDPrettySettings::kPopupPos), {pos_x_, pos_y_});
    pos_x_ = pos.x;
    pos_y_ = pos.y;
  }
}

bool OSDPretty::Supported() { return OSDPrettyWayland::SupportedOnDisplay(gdk_display_get_default() != nullptr); }

void OSDPretty::set_pos(int x, int y) {
  pos_x_ = x;
  pos_y_ = y;
}

void OSDPretty::SavePosition() const {
  Settings s;
  s.BeginGroup(OSDSettings::kSettingsGroup);
  s.SetIntValue("posx", pos_x_);
  s.SetIntValue("posy", pos_y_);
  s.BeginGroup(OSDPrettySettings::kSettingsGroup);
  s.SetValue(OSDPrettySettings::kPopupScreen, popup_screen_);
  s.SetValue(OSDPrettySettings::kPopupPos, OSDPrettyPlacement::FormatPos({pos_x_, pos_y_}));
  s.Sync();
}

std::vector<std::pair<std::string, std::string>> OSDPretty::MonitorChoices() {
  std::vector<std::pair<std::string, std::string>> choices;
  for (const auto &monitor : ListMonitors()) {
    const std::string label = monitor.name.empty() ? "Primary" : monitor.name;
    choices.emplace_back(monitor.name, label);
  }
  if (choices.empty()) {
    choices.emplace_back("", "Primary");
  }
  return choices;
}

void OSDPretty::ApplyLimits() {
  if (!window_) {
    return;
  }
  const OSDPrettyPlacement::Rect workarea = CurrentWorkarea(popup_screen_);
  const int label_width = OSDPrettyLimits::MaxLabelWidth(workarea.width);
  const int window_width = OSDPrettyLimits::MaxWindowWidth(workarea.width);
  const int window_height = OSDPrettyLimits::MaxWindowHeight(workarea.height);
  if (title_) {
    gtk_label_set_wrap(GTK_LABEL(title_), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(title_), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(title_, TRUE);
  }
  if (body_) {
    gtk_label_set_wrap(GTK_LABEL(body_), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(body_), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(body_, TRUE);
  }
  GtkCssProvider *css = gtk_css_provider_new();
  const std::string sheet = ".osd-pretty { max-width: " + std::to_string(window_width) + "px; max-height: " +
                            std::to_string(window_height) + "px; } .osd-pretty label { max-width: " +
                            std::to_string(label_width) + "px; }";
#if GTK_CHECK_VERSION(4, 12, 0)
  gtk_css_provider_load_from_string(css, sheet.c_str());
#else
  gtk_css_provider_load_from_data(css, sheet.c_str(), static_cast<gssize>(sheet.size()));
#endif
  gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css);
}

void OSDPretty::StopFade() {
  if (fade_id_) {
    g_source_remove(fade_id_);
    fade_id_ = 0;
  }
}

void OSDPretty::StartHideTimeout() {
  if (timeout_id_) {
    g_source_remove(timeout_id_);
    timeout_id_ = 0;
  }
  if (mode_ != Mode::Popup || timeout_ms_ <= 0 || disable_duration_) {
    return;
  }
  timeout_id_ = g_timeout_add(timeout_ms_, +[](gpointer data) -> gboolean {
    auto *self = static_cast<OSDPretty *>(data);
    self->timeout_id_ = 0;
    self->StartFade(false);
    return G_SOURCE_REMOVE;
  }, this);
}

void OSDPretty::StartFade(bool fading_in) {
  StopFade();
  if (!window_) {
    return;
  }
  if (!fading_) {
    gtk_widget_set_opacity(window_, fading_in ? 1.0 : 0.0);
    if (!fading_in) {
      gtk_widget_set_visible(window_, FALSE);
    } else {
      StartHideTimeout();
    }
    return;
  }
  gtk_widget_set_opacity(window_, fading_in ? 0.0 : 1.0);
  struct FadeTick {
    OSDPretty *self = nullptr;
    bool fading_in = true;
    gint64 start_us = 0;
  };
  auto *job = new FadeTick{this, fading_in, g_get_monotonic_time()};
  fade_id_ = g_timeout_add_full(
      G_PRIORITY_DEFAULT, OSDPrettyFade::kTickMs,
      +[](gpointer data) -> gboolean {
        auto *job = static_cast<FadeTick *>(data);
        if (!job->self->window_) {
          job->self->fade_id_ = 0;
          return G_SOURCE_REMOVE;
        }
        const int elapsed = static_cast<int>((g_get_monotonic_time() - job->start_us) / 1000);
        gtk_widget_set_opacity(job->self->window_, OSDPrettyFade::OpacityAt(elapsed, OSDPrettyFade::kDurationMs, job->fading_in));
        if (!OSDPrettyFade::Finished(elapsed, OSDPrettyFade::kDurationMs)) {
          return G_SOURCE_CONTINUE;
        }
        gtk_widget_set_opacity(job->self->window_, job->fading_in ? 1.0 : 0.0);
        if (!job->fading_in) {
          gtk_widget_set_visible(job->self->window_, FALSE);
        } else {
          job->self->StartHideTimeout();
        }
        job->self->fade_id_ = 0;
        return G_SOURCE_REMOVE;
      },
      job, +[](gpointer data) { delete static_cast<FadeTick *>(data); });
}

void OSDPretty::ApplyPosition() {
  const auto monitors = ListMonitors();
  std::vector<std::string> names;
  names.reserve(monitors.size());
  for (const auto &monitor : monitors) {
    names.push_back(monitor.name);
  }
  const int index = OSDPrettyPlacement::ResolveIndex(popup_screen_, names);
  const OSDPrettyPlacement::Rect workarea = (index >= 0 && static_cast<size_t>(index) < monitors.size())
                                                ? monitors[static_cast<size_t>(index)].workarea
                                                : OSDPrettyPlacement::Rect{0, 0, 1920, 1080};
  if (index >= 0 && static_cast<size_t>(index) < monitors.size()) {
    popup_screen_ = monitors[static_cast<size_t>(index)].name;
  }
  const OSDPrettyPlacement::Rect size = WindowSize(window_);
  const OSDPrettyPlacement::Point abs = OSDPrettyPlacement::AbsolutePosition(workarea, {pos_x_, pos_y_}, size.width, size.height);
  MoveWindow(window_, abs.x, abs.y);
  ApplyShape();
}

bool OSDPretty::IsTransparencyAvailable() const {
  GdkDisplay *display = gdk_display_get_default();
  if (!display) {
    return true;
  }
#ifdef HAVE_X11
  if (GDK_IS_X11_DISPLAY(display)) {
    return OSDPrettyTransparency::Available(true, gdk_display_is_composited(display), false);
  }
#endif
  return OSDPrettyTransparency::Available(false, true, true);
}

void OSDPretty::ApplyShape() {
#ifdef HAVE_X11
  if (!window_) {
    return;
  }
  GdkDisplay *display = gtk_widget_get_display(window_);
  if (!display || !GDK_IS_X11_DISPLAY(display)) {
    return;
  }
  GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window_));
  if (!surface || !GDK_IS_X11_SURFACE(surface)) {
    return;
  }
  Display *xdisplay = GDK_SURFACE_XDISPLAY(surface);
  const Window xid = gdk_x11_surface_get_xid(surface);
  const OSDPrettyPlacement::Rect size = WindowSize(window_);
  const bool transparent = IsTransparencyAvailable();
  if (OSDPrettyTransparency::ShouldClearShape(true, transparent)) {
    XRectangle rect{0, 0, static_cast<unsigned short>(std::max(1, size.width)), static_cast<unsigned short>(std::max(1, size.height))};
    XShapeCombineRectangles(xdisplay, xid, ShapeBounding, 0, 0, &rect, 1, ShapeSet, Unsorted);
    return;
  }
  if (!OSDPrettyTransparency::ShouldApplyShape(true, transparent) || size.width <= 0 || size.height <= 0) {
    return;
  }
  const std::vector<unsigned char> bits = OSDPrettyTransparency::RoundedMaskBits(size.width, size.height);
  if (bits.empty()) {
    return;
  }
  Pixmap pixmap = XCreateBitmapFromData(xdisplay, xid, reinterpret_cast<const char *>(bits.data()), static_cast<unsigned>(size.width),
                                        static_cast<unsigned>(size.height));
  if (!pixmap) {
    return;
  }
  XShapeCombineMask(xdisplay, xid, ShapeBounding, 0, 0, pixmap, ShapeSet);
  XFreePixmap(xdisplay, pixmap);
#else
  (void)0;
#endif
}

void OSDPretty::ApplyStyle() {
  if (!window_) {
    return;
  }
  GtkCssProvider *css = gtk_css_provider_new();
  const std::string sheet = OSDPrettyPopup::ChromeCss(bg_, fg_, opacity_, FontUtils::ToCss(FontUtils::Parse(font_)));
#if GTK_CHECK_VERSION(4, 12, 0)
  gtk_css_provider_load_from_string(css, sheet.c_str());
#else
  gtk_css_provider_load_from_data(css, sheet.c_str(), static_cast<gssize>(sheet.size()));
#endif
  gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css);
}

void OSDPretty::ConnectDrag() {
  if (!window_ || !OSDPrettyPopup::DragEnabled(mode_ == Mode::Draggable)) {
    return;
  }
  GtkGesture *drag = gtk_gesture_drag_new();
  gtk_widget_add_controller(window_, GTK_EVENT_CONTROLLER(drag));
  g_signal_connect(drag, "drag-begin", G_CALLBACK(OnDragBegin), this);
  g_signal_connect(drag, "drag-update", G_CALLBACK(OnDragUpdate), this);
  g_signal_connect(drag, "drag-end", G_CALLBACK(OnDragEnd), this);
}

void OSDPretty::ConnectPopup() {
  if (!window_ || !OSDPrettyPopup::ClickDismisses(mode_ == Mode::Popup)) {
    return;
  }
  GtkGesture *click = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller(window_, GTK_EVENT_CONTROLLER(click));
  g_signal_connect(click, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble, gdouble, gpointer data) {
                     static_cast<OSDPretty *>(data)->HideNow();
                   }),
                   this);
  GtkEventController *motion = gtk_event_controller_motion_new();
  gtk_widget_add_controller(window_, motion);
  g_signal_connect(motion, "enter", G_CALLBACK(+[](GtkEventControllerMotion *, gdouble, gdouble, gpointer data) {
                     static_cast<OSDPretty *>(data)->SetHoverDim(true);
                   }),
                   this);
  g_signal_connect(motion, "leave", G_CALLBACK(+[](GtkEventControllerMotion *, gpointer data) {
                     static_cast<OSDPretty *>(data)->SetHoverDim(false);
                   }),
                   this);
}

void OSDPretty::HideNow() {
  if (timeout_id_) {
    g_source_remove(timeout_id_);
    timeout_id_ = 0;
  }
  StopFade();
  if (window_) {
    gtk_widget_set_visible(window_, FALSE);
  }
}

void OSDPretty::SetHoverDim(bool dimmed) {
  if (!window_ || !OSDPrettyPopup::HoverDims(mode_ == Mode::Popup)) {
    return;
  }
  hover_dimmed_ = dimmed;
  gtk_widget_set_opacity(window_, dimmed ? OSDPrettyPopup::kHoverOpacity : 1.0);
}

void OSDPretty::SetSnapHighlight(bool snapped) {
  if (!window_ || snap_highlight_ == snapped) {
    return;
  }
  snap_highlight_ = snapped;
  if (snapped) {
    gtk_widget_add_css_class(window_, OSDPrettyPopup::kSnapClass);
  } else {
    gtk_widget_remove_css_class(window_, OSDPrettyPopup::kSnapClass);
  }
}

void OSDPretty::OnDragBegin(GtkGestureDrag *, double, double, gpointer data) {
  auto *self = static_cast<OSDPretty *>(data);
  const auto monitors = ListMonitors();
  std::vector<std::string> names;
  names.reserve(monitors.size());
  for (const auto &monitor : monitors) {
    names.push_back(monitor.name);
  }
  const int index = OSDPrettyPlacement::ResolveIndex(self->popup_screen_, names);
  const OSDPrettyPlacement::Rect workarea = (index >= 0 && static_cast<size_t>(index) < monitors.size())
                                                ? monitors[static_cast<size_t>(index)].workarea
                                                : OSDPrettyPlacement::Rect{0, 0, 1920, 1080};
  const OSDPrettyPlacement::Rect size = WindowSize(self->window_);
  const OSDPrettyPlacement::Point abs = OSDPrettyPlacement::AbsolutePosition(workarea, {self->pos_x_, self->pos_y_}, size.width, size.height, false);
  self->drag_start_x_ = abs.x;
  self->drag_start_y_ = abs.y;
}

void OSDPretty::OnDragUpdate(GtkGestureDrag *, double offset_x, double offset_y, gpointer data) {
  auto *self = static_cast<OSDPretty *>(data);
  const OSDPrettyPlacement::Point raw{static_cast<int>(self->drag_start_x_ + offset_x), static_cast<int>(self->drag_start_y_ + offset_y)};
  const auto monitors = ListMonitors();
  std::vector<OSDPrettyPlacement::Rect> rects;
  rects.reserve(monitors.size());
  for (const auto &monitor : monitors) {
    rects.push_back(monitor.workarea);
  }
  int index = OSDPrettyPlacement::IndexContaining(rects, raw);
  if (index < 0 && !monitors.empty()) {
    index = 0;
  }
  const OSDPrettyPlacement::Rect workarea = (index >= 0 && static_cast<size_t>(index) < monitors.size())
                                                ? monitors[static_cast<size_t>(index)].workarea
                                                : OSDPrettyPlacement::Rect{0, 0, 1920, 1080};
  const OSDPrettyPlacement::Rect size = WindowSize(self->window_);
  const OSDPrettyPlacement::Point abs = OSDPrettyPlacement::DragPosition(workarea, raw, size.width, size.height);
  const int center = workarea.x + workarea.width / 2 - size.width / 2;
  self->SetSnapHighlight(OSDPrettyPlacement::IsSnappedToCenter(abs.x, center));
  const OSDPrettyPlacement::Point rel = OSDPrettyPlacement::RelativePosition(workarea, abs, size.width, size.height);
  self->pos_x_ = rel.x;
  self->pos_y_ = rel.y;
  if (index >= 0 && static_cast<size_t>(index) < monitors.size()) {
    self->popup_screen_ = monitors[static_cast<size_t>(index)].name;
  }
  MoveWindow(self->window_, abs.x, abs.y);
}

void OSDPretty::OnDragEnd(GtkGestureDrag *, double, double, gpointer data) {
  auto *self = static_cast<OSDPretty *>(data);
  self->SetSnapHighlight(false);
  self->SavePosition();
}

void OSDPretty::EnsureWindow() {
  if (window_) {
    return;
  }
  window_ = gtk_window_new();
  gtk_window_set_decorated(GTK_WINDOW(window_), FALSE);
  gtk_window_set_resizable(GTK_WINDOW(window_), FALSE);
  gtk_window_set_title(GTK_WINDOW(window_), "Strawberry");
  gtk_widget_add_css_class(window_, "osd");
  gtk_widget_add_css_class(window_, "osd-pretty");
  ApplyStyle();
#ifdef HAVE_X11
  g_signal_connect(window_, "realize", G_CALLBACK(+[](GtkWidget *, gpointer data) {
                     static_cast<OSDPretty *>(data)->ApplyPosition();
                   }),
                   this);
#endif
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  image_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(image_), 48);
  GtkWidget *text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  title_ = gtk_label_new("");
  gtk_widget_add_css_class(title_, "title-3");
  gtk_label_set_xalign(GTK_LABEL(title_), 0);
  body_ = gtk_label_new("");
  gtk_widget_add_css_class(body_, "dim-label");
  gtk_label_set_xalign(GTK_LABEL(body_), 0);
  gtk_box_append(GTK_BOX(text), title_);
  gtk_box_append(GTK_BOX(text), body_);
  gtk_box_append(GTK_BOX(box), image_);
  gtk_box_append(GTK_BOX(box), text);
  gtk_window_set_child(GTK_WINDOW(window_), box);
  ConnectDrag();
  ConnectPopup();
}

void OSDPretty::SetMessage(const std::string &summary, const std::string &message, const std::vector<unsigned char> &image) {
  EnsureWindow();
  gtk_label_set_text(GTK_LABEL(title_), summary.c_str());
  gtk_label_set_text(GTK_LABEL(body_), message.c_str());
  if (OSDPrettyPopup::HideArtWhenEmpty(show_art_, !image.empty())) {
    gtk_widget_set_visible(image_, FALSE);
  } else {
    gtk_widget_set_visible(image_, TRUE);
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
    if (gdk_pixbuf_loader_write(loader, image.data(), image.size(), nullptr) && gdk_pixbuf_loader_close(loader, nullptr)) {
      GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
      if (pixbuf) {
        GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, 72, 72, GDK_INTERP_BILINEAR);
        GdkTexture *texture = gdk_texture_new_for_pixbuf(scaled);
        gtk_image_set_from_paintable(GTK_IMAGE(image_), GDK_PAINTABLE(texture));
        g_object_unref(texture);
        g_object_unref(scaled);
      }
    }
    g_object_unref(loader);
  }
}

void OSDPretty::ShowMessage(const std::string &summary, const std::string &message, const std::vector<unsigned char> &image) {
  EnsureWindow();
  const bool visible = gtk_widget_get_visible(window_);
  if (OSDPrettyPopup::ShouldHideOnRepeat(visible, mode_ == Mode::Popup, toggle_mode_)) {
    toggle_mode_ = false;
    HideNow();
    return;
  }
  SetMessage(summary, message, image);
  ApplyLimits();
  if (OSDPrettyPopup::ShouldRestartTimeout(visible, mode_ == Mode::Popup, toggle_mode_)) {
    StartHideTimeout();
    return;
  }
  toggle_mode_ = false;
  if (timeout_id_) {
    g_source_remove(timeout_id_);
    timeout_id_ = 0;
  }
  StopFade();
  if (fading_) {
    gtk_widget_set_opacity(window_, 0.0);
  }
  gtk_window_present(GTK_WINDOW(window_));
  ApplyPosition();
  ApplyLimits();
  StartFade(true);
}
