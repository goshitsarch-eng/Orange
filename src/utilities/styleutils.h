#ifndef STRAWBERRY_STYLEUTILS_H
#define STRAWBERRY_STYLEUTILS_H

#include <gtk/gtk.h>
#include <string>

namespace StyleUtils {

void LoadCss(const std::string &css);
bool IsDarkTheme();

}  // namespace StyleUtils

#endif
