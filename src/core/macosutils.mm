#include "core/macosutils.h"

#include "core/macoswindow.h"

#ifdef __APPLE__
#include <AppKit/AppKit.h>

void MacOsUtils::SetApplicationName(const char *name) {
  if (!name) {
    return;
  }
  [[NSProcessInfo processInfo] setProcessName:[NSString stringWithUTF8String:name]];
}

void MacOsUtils::EnableFullScreen(GtkWindow *window) {
  (void)window;
  if (!MacOsWindow::ShouldEnableFullScreen()) {
    return;
  }
  NSWindow *nswindow = [NSApp keyWindow];
  if (!nswindow) {
    nswindow = [NSApp mainWindow];
  }
  if (!nswindow) {
    return;
  }
  [nswindow setCollectionBehavior:[nswindow collectionBehavior] | static_cast<NSWindowCollectionBehavior>(MacOsWindow::FullScreenPrimaryMask())];
}
#endif
