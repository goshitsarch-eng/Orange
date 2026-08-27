#ifndef STRAWBERRY_ICONLOADER_H
#define STRAWBERRY_ICONLOADER_H

#include <gtk/gtk.h>

#include <string>
#include <vector>

class IconLoader {
 public:
  static GdkPixbuf *Load(const std::string &name, int size = 32);
  static std::string ThemeName();
  static std::vector<std::string> SearchNames(const std::string &name);
};

#endif
