#ifndef STRAWBERRY_WINUTILS_H
#define STRAWBERRY_WINUTILS_H

#ifdef _WIN32
#include <gtk/gtk.h>
#endif

namespace WinUtils {

#ifdef _WIN32
void EnableBlurBehindWindow(GtkWidget *window);
void *NativeHandle(GtkWidget *window);
#endif

}  // namespace WinUtils

#endif
