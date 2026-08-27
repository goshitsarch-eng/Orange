#ifndef STRAWBERRY_STYLEHELPER_H
#define STRAWBERRY_STYLEHELPER_H

#include "utilities/styleutils.h"

class StyleHelper {
 public:
  static void LoadCss(const std::string &css) { StyleUtils::LoadCss(css); }
  static bool IsDark() { return StyleUtils::IsDarkTheme(); }
};

#endif
