#include "utilities/winutils.h"

#ifdef _WIN32
#include <dwmapi.h>
#include <windows.h>
#ifdef GDK_WINDOWING_WIN32
#include <gdk/win32/gdkwin32.h>
#endif

void *WinUtils::NativeHandle(GtkWidget *window) {
  if (!window) {
    return nullptr;
  }
  GtkNative *native = gtk_widget_get_native(window);
  if (!native) {
    return nullptr;
  }
  GdkSurface *surface = gtk_native_get_surface(native);
#ifdef GDK_WINDOWING_WIN32
  if (surface && GDK_IS_WIN32_SURFACE(surface)) {
    return gdk_win32_surface_get_handle(surface);
  }
#endif
  (void)surface;
  return nullptr;
}

void WinUtils::EnableBlurBehindWindow(GtkWidget *window) {
  HWND hwnd = static_cast<HWND>(NativeHandle(window));
  if (!hwnd) {
    return;
  }
  DWM_BLURBEHIND dwmbb{};
  dwmbb.dwFlags = DWM_BB_ENABLE;
  dwmbb.fEnable = TRUE;
  DwmEnableBlurBehindWindow(hwnd, &dwmbb);
}
#endif
