#include "utilities/imageutils.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

namespace ImageUtils {

bool IsJpeg(const std::string &data) {
  return data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xff && static_cast<unsigned char>(data[1]) == 0xd8;
}

bool IsPng(const std::string &data) { return data.size() >= 8 && data.compare(0, 8, "\x89PNG\r\n\x1a\n") == 0; }

std::vector<unsigned char> ScaleIfNeeded(const std::vector<unsigned char> &data, int max_size) {
  if (data.empty() || max_size <= 0) {
    return data;
  }
  GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
  if (!gdk_pixbuf_loader_write(loader, data.data(), data.size(), nullptr) || !gdk_pixbuf_loader_close(loader, nullptr)) {
    g_object_unref(loader);
    return data;
  }
  GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
  if (!pixbuf) {
    g_object_unref(loader);
    return data;
  }
  const int width = gdk_pixbuf_get_width(pixbuf);
  const int height = gdk_pixbuf_get_height(pixbuf);
  if (width <= max_size && height <= max_size) {
    g_object_unref(loader);
    return data;
  }
  const double scale = static_cast<double>(max_size) / (width > height ? width : height);
  GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, static_cast<int>(width * scale), static_cast<int>(height * scale), GDK_INTERP_BILINEAR);
  gchar *buffer = nullptr;
  gsize size = 0;
  gdk_pixbuf_save_to_buffer(scaled, &buffer, &size, "png", nullptr, nullptr);
  std::vector<unsigned char> out;
  if (buffer && size) {
    out.assign(buffer, buffer + size);
  }
  g_free(buffer);
  g_object_unref(scaled);
  g_object_unref(loader);
  return out.empty() ? data : out;
}

}  // namespace ImageUtils
