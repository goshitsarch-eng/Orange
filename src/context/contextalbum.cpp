#include "context/contextalbum.h"

#include "translations/translations.h"
#include "utilities/fileutils.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <cstring>

ContextAlbum::ContextAlbum() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  image_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(image_), 220);
  gtk_widget_set_halign(image_, GTK_ALIGN_CENTER);
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
  gtk_box_append(GTK_BOX(widget_), image_);
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
}

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
  gtk_spinner_stop(GTK_SPINNER(spinner_));
  gtk_widget_set_visible(spinner_, FALSE);
  gtk_image_set_from_icon_name(GTK_IMAGE(image_), "audio-x-generic-symbolic");
}

void ContextAlbum::SetSearchCallback(SearchCallback callback) { search_ = std::move(callback); }

void ContextAlbum::SetDropCallback(DropCallback callback) { drop_ = std::move(callback); }

void ContextAlbum::SearchCoverInProgress() {
  downloading_ = true;
  gtk_widget_set_visible(spinner_, TRUE);
  gtk_spinner_start(GTK_SPINNER(spinner_));
}

void ContextAlbum::SetImage(const std::vector<unsigned char> &data, int pixel_size) {
  downloading_ = false;
  gtk_spinner_stop(GTK_SPINNER(spinner_));
  gtk_widget_set_visible(spinner_, FALSE);
  if (data.empty()) {
    Clear();
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
    return;
  }
  gtk_image_set_from_paintable(GTK_IMAGE(image_), GDK_PAINTABLE(texture));
  gtk_image_set_pixel_size(GTK_IMAGE(image_), pixel_size);
  g_object_unref(texture);
}
