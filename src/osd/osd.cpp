#include "osd/osd.h"

#include "config.h"
#include "core/settings.h"

#include <algorithm>

#include <gdk-pixbuf/gdk-pixbuf.h>
#ifdef HAVE_X11
#include <gdk/x11/gdkx.h>
#include <X11/Xlib.h>
#endif

void OSD::ReloadSettings() {
  Settings s;
  s.BeginGroup("OSD");
  enabled_ = s.BoolValue("enabled", true);
  show_art_ = s.BoolValue("showart", true);
  type_ = s.Value("type", "native");
  timeout_ms_ = s.IntValue("timeout", 4000);
  fg_ = s.Value("foreground", "#ffffff");
  bg_ = s.Value("background", "#202020");
  opacity_ = s.DoubleValue("opacity", 0.92);
  pos_x_ = s.IntValue("posx", 40);
  pos_y_ = s.IntValue("posy", 40);
  font_ = s.Value("font", "Sans 12");
}

void OSD::ShowMessage(const std::string &summary, const std::string &body, const std::string &icon) {
  if (!enabled_) {
    return;
  }
  GNotification *notification = g_notification_new(summary.c_str());
  g_notification_set_body(notification, body.c_str());
  GIcon *gicon = g_themed_icon_new(icon.c_str());
  g_notification_set_icon(notification, gicon);
  g_object_unref(gicon);
  if (GApplication *app = g_application_get_default()) {
    g_application_send_notification(app, "strawberry-osd", notification);
  }
  g_object_unref(notification);
}

void OSD::ShowPretty(const std::string &summary, const std::string &body, const std::vector<unsigned char> &art) {
  if (pretty_timeout_) {
    g_source_remove(pretty_timeout_);
    pretty_timeout_ = 0;
  }
  if (pretty_window_) {
    gtk_window_destroy(GTK_WINDOW(pretty_window_));
    pretty_window_ = nullptr;
  }
  pretty_window_ = gtk_window_new();
  gtk_window_set_decorated(GTK_WINDOW(pretty_window_), FALSE);
  gtk_window_set_resizable(GTK_WINDOW(pretty_window_), FALSE);
  gtk_window_set_title(GTK_WINDOW(pretty_window_), "Strawberry");
  gtk_widget_add_css_class(pretty_window_, "osd");
  gtk_widget_add_css_class(pretty_window_, "osd-pretty");
  GtkCssProvider *css = gtk_css_provider_new();
  const std::string sheet = ".osd-pretty { background-color: alpha(" + bg_ + ", " + std::to_string(std::clamp(opacity_, 0.2, 1.0)) +
                            "); color: " + fg_ + "; font: " + font_ + "; border-radius: 10px; }";
#if GTK_CHECK_VERSION(4, 12, 0)
  gtk_css_provider_load_from_string(css, sheet.c_str());
#else
  gtk_css_provider_load_from_data(css, sheet.c_str(), static_cast<gssize>(sheet.size()));
#endif
  gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css);
  g_object_set_data(G_OBJECT(pretty_window_), "pos-x", GINT_TO_POINTER(pos_x_));
  g_object_set_data(G_OBJECT(pretty_window_), "pos-y", GINT_TO_POINTER(pos_y_));
#ifdef HAVE_X11
  g_signal_connect(pretty_window_, "realize", G_CALLBACK(+[](GtkWidget *window, gpointer) {
                     GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
                     if (surface && GDK_IS_X11_SURFACE(surface)) {
                       const int x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window), "pos-x"));
                       const int y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window), "pos-y"));
                       XMoveWindow(GDK_SURFACE_XDISPLAY(surface), gdk_x11_surface_get_xid(surface), x, y);
                     }
                   }),
                   nullptr);
#endif
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  if (show_art_ && !art.empty()) {
    GBytes *bytes = g_bytes_new(art.data(), art.size());
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
    gsize size = 0;
    const void *data = g_bytes_get_data(bytes, &size);
    if (gdk_pixbuf_loader_write(loader, static_cast<const guchar *>(data), size, nullptr) && gdk_pixbuf_loader_close(loader, nullptr)) {
      GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
      if (pixbuf) {
        GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, 72, 72, GDK_INTERP_BILINEAR);
        GdkTexture *texture = gdk_texture_new_for_pixbuf(scaled);
        GtkWidget *image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));
        g_object_unref(texture);
        gtk_box_append(GTK_BOX(box), image);
        g_object_unref(scaled);
      }
    }
    g_object_unref(loader);
    g_bytes_unref(bytes);
  } else {
    GtkWidget *image = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(image), 48);
    gtk_box_append(GTK_BOX(box), image);
  }
  GtkWidget *text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  GtkWidget *title = gtk_label_new(summary.c_str());
  gtk_widget_add_css_class(title, "title-3");
  gtk_label_set_xalign(GTK_LABEL(title), 0);
  GtkWidget *subtitle = gtk_label_new(body.c_str());
  gtk_widget_add_css_class(subtitle, "dim-label");
  gtk_label_set_xalign(GTK_LABEL(subtitle), 0);
  gtk_box_append(GTK_BOX(text), title);
  gtk_box_append(GTK_BOX(text), subtitle);
  gtk_box_append(GTK_BOX(box), text);
  gtk_window_set_child(GTK_WINDOW(pretty_window_), box);
  gtk_window_present(GTK_WINDOW(pretty_window_));
  pretty_timeout_ = g_timeout_add(timeout_ms_ > 0 ? timeout_ms_ : 4000, [](gpointer data) -> gboolean {
    auto *self = static_cast<OSD *>(data);
    if (self->pretty_window_) {
      gtk_window_destroy(GTK_WINDOW(self->pretty_window_));
      self->pretty_window_ = nullptr;
    }
    self->pretty_timeout_ = 0;
    return G_SOURCE_REMOVE;
  }, this);
}

void OSD::SongChanged(const Song &song, const std::vector<unsigned char> &art) {
  const std::string body = song.EffectiveAlbumartist() + (song.album().empty() ? "" : "\n" + song.album());
  if (type_ == "pretty" || type_ == "both") {
    ShowPretty(song.PrettyTitle(), body, art);
  }
  if (type_ == "native" || type_ == "both" || type_.empty()) {
    ShowMessage(song.PrettyTitle(), body);
  }
}
