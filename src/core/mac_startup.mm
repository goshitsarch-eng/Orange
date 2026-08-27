#include "core/mac_startup.h"

#ifdef __APPLE__
#include "core/macosutils.h"

#import <AppKit/AppKit.h>

void MacStartup() {
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  MacOsUtils::SetApplicationName("Strawberry");
}
#endif
