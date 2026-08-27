#include "context/contextalbum.h"

#include "translations/translations.h"
#include "utilities/fileutils.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <cstring>

ContextAlbum::ContextAlbum() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  previous_image_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(previous_image_), 220);
  gtk_widget_set_halign(previous_image_, GTK_ALIGN_CENTER);
  gtk_widget_set_visible(previous_image_, FALSE);
  image_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(image_), 220);
  gtk_widget_set_halign(image_, GTK_ALIGN_CENTER);
  GtkWidget *overlay = gtk_overlay_new();
  gtk_overlay_set_child(GTK_OVERLAY(overlay), previous_image_);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), image_);
  gtk_widget_set_halign(overlay, GTK_ALIGN_CENTER);
  spinner_ = gtk_spinner_new();
  gtk_widget_set_halign(spinner_, GTK_ALIGN_CENTER);
  gtk_widget_set_visible(spinner_, FALSE);
  GtkWidget *search = gtk_button_new_with_label(Translations::CStr("Search cover"));
  gtk_widget_add_css_class(search, "flat");
  gtk_widget_set_halign(search, GTK_ALIGN_CENTER);
  g_signal_connect(search, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<ContextAlbum *>(data);
                     if (self->search_) {
                       self->search_();
                     }
                   }),
                   this);
  GtkWidget *sensor = gtk_drawing_area_new();
  gtk_widget_set_hexpand(sensor, TRUE);
  gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(sensor), 1);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(sensor), +[](GtkDrawingArea *, cairo_t *, int, int, gpointer) {}, nullptr, nullptr);
  g_signal_connect(sensor, "resize", G_CALLBACK(+[](GtkDrawingArea *, gint width, gint, gpointer data) {
                     static_cast<ContextAlbum *>(data)->UpdateWidth(width);
                   }),
                   this);
  gtk_box_append(GTK_BOX(widget_), sensor);
  gtk_box_append(GTK_BOX(widget_), overlay);
  gtk_box_append(GTK_BOX(widget_), spinner_);
  gtk_box_append(GTK_BOX(widget_), search);

  GtkDropTarget *target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
#ifdef GDK_TYPE_FILE_LIST
  GType types[] = {G_TYPE_STRING, GDK_TYPE_FILE_LIST};
  gtk_drop_target_set_gtypes(target, types, 2);
#endif
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(target));
  g_signal_connect(target, "drop", G_CALLBACK(+[](GtkDropTarget *, const GValue *value, gdouble, gdouble, gpointer data) -> gboolean {
                     return static_cast<ContextAlbum *>(data)->OnDrop(value);
                   }),
                   this);

  GtkGesture *activate = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(activate), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(activate));
  g_signal_connect(activate, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint n_press, gdouble, gdouble, gpointer data) {
                     auto *self = static_cast<ContextAlbum *>(data);
                     if (n_press == 2 && self->has_cover_ && self->activate_) {
                       self->activate_();
                     }
                   }),
                   this);
}

ContextAlbum::~ContextAlbum() { StopFade(); }

gboolean ContextAlbum::OnDrop(const GValue *value) {
  std::vector<std::string> paths;
  if (G_VALUE_HOLDS_STRING(value)) {
    const char *text = g_value_get_string(value);
    for (const std::string &part : StrUtils::Split(text ? text : "", '\n')) {
      std::string url = part;
      if (!url.empty() && url.back() == '\r') {
        url.pop_back();
      }
      if (!url.empty()) {
        paths.push_back(url);
      }
    }
  }
#ifdef GDK_TYPE_FILE_LIST
  if (G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST)) {
    auto *list = static_cast<GdkFileList *>(g_value_get_boxed(value));
    GSList *files = gdk_file_list_get_files(list);
    for (GSList *item = files; item; item = item->next) {
      gchar *uri = g_file_get_uri(G_FILE(item->data));
      if (uri) {
        paths.emplace_back(uri);
        g_free(uri);
      }
    }
  }
#endif
  for (const std::string &url : paths) {
    const std::string path = FileUtils::PathFromUri(url);
    if (!IsImagePath(path) && !IsImagePath(url)) {
      continue;
    }
    const std::string data = FileUtils::ReadFile(path);
    if (data.empty() || !JsonUtils::LooksLikeImage(data)) {
      continue;
    }
    if (drop_) {
      drop_(std::vector<unsigned char>(data.begin(), data.end()));
    }
    return TRUE;
  }
  return FALSE;
}

void ContextAlbum::Clear() {
  downloading_ = false;
  has_cover_ = false;
  fading_to_placeholder_ = false;
  StopFade();
  gtk_spinner_stop(GTK_SPINNER(spinner_));
  gtk_widget_set_visible(spinner_, FALSE);
  gtk_widget_set_opacity(image_, 1.0);
  gtk_widget_set_visible(previous_image_, FALSE);
  gtk_image_set_from_icon_name(GTK_IMAGE(previous_image_), "audio-x-generic-symbolic");
  gtk_image_set_from_icon_name(GTK_IMAGE(image_), "audio-x-generic-symbolic");
}

void ContextAlbum::SetSearchCallback(SearchCallback callback) { search_ = std::move(callback); }

void ContextAlbum::SetDropCallback(DropCallback callback) { drop_ = std::move(callback); }

void ContextAlbum::SetFadeFinishedCallback(FadeFinishedCallback callback) { fade_finished_ = std::move(callback); }

void ContextAlbum::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void ContextAlbum::UpdateWidth(int allocated_width) {
  const int size = CoverPixelSize(allocated_width);
  if (image_) {
    gtk_image_set_pixel_size(GTK_IMAGE(image_), size);
  }
  if (previous_image_) {
    gtk_image_set_pixel_size(GTK_IMAGE(previous_image_), size);
  }
}

void ContextAlbum::SearchCoverInProgress() {
  downloading_ = true;
  gtk_widget_set_visible(spinner_, TRUE);
  gtk_spinner_start(GTK_SPINNER(spinner_));
}

void ContextAlbum::SnapshotCurrentToPrevious() {
  GdkPaintable *paintable = gtk_image_get_paintable(GTK_IMAGE(image_));
  if (paintable) {
    gtk_image_set_from_paintable(GTK_IMAGE(previous_image_), paintable);
  } else {
    gtk_image_set_from_icon_name(GTK_IMAGE(previous_image_), "audio-x-generic-symbolic");
  }
  gtk_image_set_pixel_size(GTK_IMAGE(previous_image_), gtk_image_get_pixel_size(GTK_IMAGE(image_)));
  gtk_widget_set_visible(previous_image_, TRUE);
  gtk_widget_set_opacity(previous_image_, 1.0);
}

void ContextAlbum::ApplyImageData(const std::vector<unsigned char> &data, int pixel_size) {
  if (data.empty()) {
    gtk_image_set_from_icon_name(GTK_IMAGE(image_), "audio-x-generic-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(image_), pixel_size);
    return;
  }
  GBytes *bytes = g_bytes_new(data.data(), data.size());
  GError *error = nullptr;
  GdkTexture *texture = gdk_texture_new_from_bytes(bytes, &error);
  g_bytes_unref(bytes);
  if (!texture) {
    if (error) {
      g_error_free(error);
    }
    gtk_image_set_from_icon_name(GTK_IMAGE(image_), "audio-x-generic-symbolic");
    return;
  }
  gtk_image_set_from_paintable(GTK_IMAGE(image_), GDK_PAINTABLE(texture));
  gtk_image_set_pixel_size(GTK_IMAGE(image_), pixel_size);
  g_object_unref(texture);
}

void ContextAlbum::StartFade(bool to_placeholder) {
  StopFade();
  fading_to_placeholder_ = to_placeholder;
  fade_elapsed_ms_ = 0;
  gtk_widget_set_opacity(image_, 0.0);
  fade_timeout_id_ = g_timeout_add(kFadeTickMs, [](gpointer data) -> gboolean { return static_cast<ContextAlbum *>(data)->FadeTick(); }, this);
}

void ContextAlbum::StopFade() {
  if (fade_timeout_id_) {
    g_source_remove(fade_timeout_id_);
    fade_timeout_id_ = 0;
  }
  fade_elapsed_ms_ = 0;
}

gboolean ContextAlbum::FadeTick() {
  fade_elapsed_ms_ += kFadeTickMs;
  const double fade_in = FadeInOpacity(fade_elapsed_ms_);
  gtk_widget_set_opacity(previous_image_, FadeOutOpacity(fade_elapsed_ms_));
  gtk_widget_set_opacity(image_, fade_in);
  if (fade_elapsed_ms_ < kFadeTimelineMs) {
    return G_SOURCE_CONTINUE;
  }
  fade_timeout_id_ = 0;
  gtk_widget_set_opacity(image_, 1.0);
  gtk_widget_set_visible(previous_image_, FALSE);
  gtk_image_set_from_icon_name(GTK_IMAGE(previous_image_), "audio-x-generic-symbolic");
  if (fading_to_placeholder_ && fade_finished_) {
    fade_finished_();
  }
  fading_to_placeholder_ = false;
  return G_SOURCE_REMOVE;
}

void ContextAlbum::SetImage(const std::vector<unsigned char> &data, int pixel_size) {
  downloading_ = false;
  gtk_spinner_stop(GTK_SPINNER(spinner_));
  gtk_widget_set_visible(spinner_, FALSE);
  SnapshotCurrentToPrevious();
  has_cover_ = !data.empty();
  ApplyImageData(data, pixel_size);
  gtk_widget_set_opacity(image_, 0.0);
  StartFade(!has_cover_);
}
