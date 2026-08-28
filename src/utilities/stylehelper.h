#ifndef STRAWBERRY_STYLEHELPER_H
#define STRAWBERRY_STYLEHELPER_H

#include "utilities/styleutils.h"

class StyleHelper {
 public:
  static void LoadCss(const std::string &css, const std::string &slot) { StyleUtils::LoadCss(css, slot); }
  static bool IsDark() { return StyleUtils::IsDarkTheme(); }
};

#endif
