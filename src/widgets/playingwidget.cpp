#include "widgets/playingwidget.h"

PlayingWidget::PlayingWidget() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  cover_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(cover_), 48);
  GtkWidget *labels = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  title_ = gtk_label_new("Not playing");
  gtk_widget_add_css_class(title_, "heading");
  gtk_widget_set_halign(title_, GTK_ALIGN_START);
  artist_ = gtk_label_new("");
  gtk_widget_add_css_class(artist_, "dim-label");
  gtk_widget_set_halign(artist_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(labels), title_);
  gtk_box_append(GTK_BOX(labels), artist_);
  gtk_box_append(GTK_BOX(widget_), cover_);
  gtk_box_append(GTK_BOX(widget_), labels);
}

void PlayingWidget::SetEnabled(bool enabled) {
  enabled_ = enabled;
  gtk_widget_set_visible(widget_, enabled);
}

void PlayingWidget::SongChanged(const Song &song) {
  song_ = song;
  gtk_label_set_text(GTK_LABEL(title_), song.PrettyTitle().empty() ? "Not playing" : song.PrettyTitle().c_str());
  gtk_label_set_text(GTK_LABEL(artist_), song.EffectiveAlbumartist().c_str());
}

void PlayingWidget::SetCover(const std::vector<unsigned char> &data) { SetImageFromBytes(data); }

void PlayingWidget::SetImageFromBytes(const std::vector<unsigned char> &data) {
  if (data.empty()) {
    gtk_image_set_from_icon_name(GTK_IMAGE(cover_), "audio-x-generic-symbolic");
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
  gtk_image_set_from_paintable(GTK_IMAGE(cover_), GDK_PAINTABLE(texture));
  gtk_image_set_pixel_size(GTK_IMAGE(cover_), 48);
  g_object_unref(texture);
}
