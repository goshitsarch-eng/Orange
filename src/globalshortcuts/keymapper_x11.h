#ifndef STRAWBERRY_KEYMAPPER_X11_H
#define STRAWBERRY_KEYMAPPER_X11_H

#include <string>

class KeyMapperX11 {
 public:
  static unsigned long KeysymFromName(const std::string &name);
  static std::string NameFromKeysym(unsigned long keysym);
};

#endif
