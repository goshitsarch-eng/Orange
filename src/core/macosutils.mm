#include "core/macosutils.h"

#ifdef __APPLE__
#include <AppKit/AppKit.h>

void MacOsUtils::SetApplicationName(const char *name) {
  if (!name) {
    return;
  }
  [[NSProcessInfo processInfo] setProcessName:[NSString stringWithUTF8String:name]];
}

void MacOsUtils::EnableFullScreen(GtkWindow *) {}
#endif
