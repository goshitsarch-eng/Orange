#ifndef STRAWBERRY_STYLESHEETLOADER_H
#define STRAWBERRY_STYLESHEETLOADER_H

#include <string>

class StyleSheetLoader {
 public:
  static bool LoadFile(const std::string &path);
};

#endif
