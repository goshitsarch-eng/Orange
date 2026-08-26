#include "dialogs/dialoghelpers.h"

#include "covermanager/coverproviders.h"
#include "utilities/fileutils.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <ctime>

namespace DialogHelpers {

Song SongForDialog(Application *app) {
  Song song = app->player()->current_song();
  if (app->playlist_manager()->active() && app->playlist_manager()->current_row() >= 0) {
    song = app->playlist_manager()->current_song();
  }
  return song;
}

void SetImageFromBytes(GtkWidget *image, const std::vector<unsigned char> &data, int pixel_size) {
  if (!image) {
    return;
  }
  if (data.empty()) {
    gtk_image_set_from_icon_name(GTK_IMAGE(image), "audio-x-generic-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(image), pixel_size);
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

std::string PrettyBytes(int64_t bytes) {
  if (bytes < 0) {
    return {};
  }
  if (bytes < 1024) {
    return std::to_string(bytes) + " B";
  }
  if (bytes < 1024 * 1024) {
    return std::to_string(bytes / 1024) + " KB";
  }
  char buf[32];
  g_snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(bytes) / 1048576.0);
  return buf;
}

std::string PrettyUnixTime(int64_t ts) {
  if (ts <= 0) {
    return "Never";
  }
  const time_t value = static_cast<time_t>(ts);
  struct tm local {};
  localtime_r(&value, &local);
  char buf[64];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &local);
  return buf;
}

std::string SafeFolderName(std::string name) {
  for (char &ch : name) {
    if (ch == '/' || ch == '\\' || ch == ':' || ch == '\0') {
      ch = '-';
    }
  }
  return name.empty() ? "Unknown" : name;
}

GtkWidget *DropDownFromNames(const std::vector<std::string> &names) {
  GtkStringList *list = gtk_string_list_new(nullptr);
  for (const std::string &name : names) {
    gtk_string_list_append(list, name.c_str());
  }
  return gtk_drop_down_new(G_LIST_MODEL(list), nullptr);
}

bool ApplyCover(Application *app, Song *song, const std::string &image) {
  if (!song || image.empty() || !CoverProviders::SaveAlbumCover(*song, image, app->tagreader())) {
    return false;
  }
  const std::string dest = FileUtils::Join(FileUtils::DirName(FileUtils::PathFromUri(song->url())), "cover.jpg");
  song->set_art_manual(FileUtils::UriFromPath(dest));
  song->set_art_unset(false);
  song->set_art_embedded(true);
  if (song->id() > 0) {
    app->collection()->backend()->AddOrUpdateSong(*song);
  }
  return true;
}

}  // namespace DialogHelpers
