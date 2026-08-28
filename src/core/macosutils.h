#ifndef STRAWBERRY_MACOSUTILS_H
#define STRAWBERRY_MACOSUTILS_H

#ifdef __APPLE__
#include <gtk/gtk.h>
#endif

namespace MacOsUtils {

#ifdef __APPLE__
void SetApplicationName(const char *name);
void EnableFullScreen(GtkWindow *window);
#endif

}  // namespace MacOsUtils

#endif
