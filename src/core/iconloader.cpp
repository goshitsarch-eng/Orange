#include "core/iconloader.h"

std::string IconLoader::ThemeName() {
  GtkIconTheme *theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
  const char *name = theme ? gtk_icon_theme_get_theme_name(theme) : nullptr;
  return name ? name : "Adwaita";
}

std::vector<std::string> IconLoader::SearchNames(const std::string &name) {
  return {name, name + "-symbolic", "audio-x-generic", "audio-x-generic-symbolic"};
}

GdkPixbuf *IconLoader::Load(const std::string &name, int size) {
  GtkIconTheme *theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
  if (!theme) {
    return nullptr;
  }
  for (const std::string &candidate : SearchNames(name)) {
    GtkIconPaintable *paintable = gtk_icon_theme_lookup_icon(theme, candidate.c_str(), nullptr, size, 1, GTK_TEXT_DIR_NONE, static_cast<GtkIconLookupFlags>(0));
    if (!paintable) {
      continue;
    }
    GFile *file = gtk_icon_paintable_get_file(paintable);
    g_object_unref(paintable);
    if (!file) {
      continue;
    }
    gchar *path = g_file_get_path(file);
    g_object_unref(file);
    if (!path) {
      continue;
    }
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(path, size, size, nullptr);
    g_free(path);
    if (pixbuf) {
      return pixbuf;
    }
  }
  return nullptr;
}
