#ifndef STRAWBERRY_STANDARDITEMICONLOADER_H
#define STRAWBERRY_STANDARDITEMICONLOADER_H

#include "core/iconloader.h"

class StandardItemIconLoader {
 public:
  static GdkPixbuf *Load(const std::string &name, int size = 16) { return IconLoader::Load(name, size); }
};

#endif
