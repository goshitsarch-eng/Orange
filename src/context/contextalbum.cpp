#include "context/contextalbum.h"

ContextAlbum::ContextAlbum() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  image_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(image_), 220);
  gtk_widget_set_halign(image_, GTK_ALIGN_CENTER);
  GtkWidget *search = gtk_button_new_with_label("Search cover");
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
  gtk_box_append(GTK_BOX(widget_), search);
}

void ContextAlbum::Clear() {
  downloading_ = false;
  gtk_image_set_from_icon_name(GTK_IMAGE(image_), "audio-x-generic-symbolic");
}

void ContextAlbum::SetSearchCallback(SearchCallback callback) { search_ = std::move(callback); }

void ContextAlbum::SearchCoverInProgress() { downloading_ = true; }

void ContextAlbum::SetImage(const std::vector<unsigned char> &data, int pixel_size) {
  downloading_ = false;
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
