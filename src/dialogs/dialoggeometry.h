#ifndef STRAWBERRY_DIALOGGEOMETRY_H
#define STRAWBERRY_DIALOGGEOMETRY_H

#include "core/settings.h"

#include <string>

#include <adwaita.h>

namespace DialogGeometry {

constexpr int kMinSize = 200;

inline std::string Encode(int width, int height) { return std::to_string(width) + "x" + std::to_string(height); }

inline bool Decode(const std::string &value, int *width, int *height) {
  if (!width || !height || value.empty()) {
    return false;
  }
  const std::string::size_type x = value.find('x');
  if (x == std::string::npos || x == 0 || x + 1 >= value.size()) {
    return false;
  }
  try {
    const int w = std::stoi(value.substr(0, x));
    const int h = std::stoi(value.substr(x + 1));
    if (w < kMinSize || h < kMinSize) {
      return false;
    }
    *width = w;
    *height = h;
    return true;
  } catch (...) {
    return false;
  }
}

inline bool ShouldRestore(const std::string &value) {
  int width = 0;
  int height = 0;
  return Decode(value, &width, &height);
}

struct Size {
  int width = 0;
  int height = 0;
};

inline Size RestoreOrDefault(const std::string &stored, int fallback_width, int fallback_height) {
  Size size{fallback_width, fallback_height};
  Decode(stored, &size.width, &size.height);
  return size;
}

inline void Apply(AdwDialog *dialog, const char *group, const char *key, int fallback_width, int fallback_height,
                  bool apply_default_height = true) {
  if (!dialog || !group || !key) {
    return;
  }
  Settings settings;
  settings.BeginGroup(group);
  int width = fallback_width;
  int height = fallback_height;
  const bool stored = Decode(settings.Value(key), &width, &height);
  adw_dialog_set_content_width(dialog, stored ? width : fallback_width);
  if (stored || apply_default_height) {
    adw_dialog_set_content_height(dialog, stored ? height : fallback_height);
  }
}

inline void Save(AdwDialog *dialog, const char *group, const char *key) {
  if (!dialog || !group || !key) {
    return;
  }
  Settings settings;
  settings.BeginGroup(group);
  settings.SetValue(key, Encode(adw_dialog_get_content_width(dialog), adw_dialog_get_content_height(dialog)));
  settings.Sync();
}

inline void BindClosed(AdwDialog *dialog, const char *group, const char *key) {
  if (!dialog || !group || !key) {
    return;
  }
  g_object_set_data_full(G_OBJECT(dialog), "geometry-group", g_strdup(group), g_free);
  g_object_set_data_full(G_OBJECT(dialog), "geometry-key", g_strdup(key), g_free);
  g_signal_connect(dialog, "closed", G_CALLBACK((+[](AdwDialog *closed, gpointer) {
                     const char *group = static_cast<const char *>(g_object_get_data(G_OBJECT(closed), "geometry-group"));
                     const char *key = static_cast<const char *>(g_object_get_data(G_OBJECT(closed), "geometry-key"));
                     Save(closed, group, key);
                   })),
                   nullptr);
}

}  // namespace DialogGeometry

#endif
