#include "core/mac_startup.h"

#ifdef __APPLE__
#include "core/macstartupactions.h"
#include "core/macosutils.h"
#include "core/platforminterface.h"
#include "globalshortcuts/globalshortcutsbackend-macos.h"

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>

static PlatformInterface *g_app_handler = nullptr;
static GlobalShortcutsBackendMacOs *g_shortcut_handler = nullptr;
static id g_gtk_delegate = nil;
static NSMenu *g_dock_menu = nil;

@interface StrawberryAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation StrawberryAppDelegate

- (BOOL)respondsToSelector:(SEL)sel {
  if ([super respondsToSelector:sel]) {
    return YES;
  }
  return [g_gtk_delegate respondsToSelector:sel];
}

- (id)forwardingTargetForSelector:(SEL)sel {
  if (g_gtk_delegate && [g_gtk_delegate respondsToSelector:sel]) {
    return g_gtk_delegate;
  }
  return [super forwardingTargetForSelector:sel];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)app hasVisibleWindows:(BOOL)flag {
  if (g_app_handler && MacStartupActions::ShouldHandleReopen()) {
    g_app_handler->Activate();
  }
  if (g_gtk_delegate && [g_gtk_delegate respondsToSelector:@selector(applicationShouldHandleReopen:hasVisibleWindows:)]) {
    return [g_gtk_delegate applicationShouldHandleReopen:app hasVisibleWindows:flag];
  }
  return MacStartupActions::ReopenReturnYes() ? YES : NO;
}

- (NSMenu *)applicationDockMenu:(NSApplication *)sender {
  (void)sender;
  return g_dock_menu;
}

- (BOOL)application:(NSApplication *)app openFile:(NSString *)filename {
  (void)app;
  if (!g_app_handler || !filename) {
    return NO;
  }
  return g_app_handler->LoadUrl([filename UTF8String]) ? YES : NO;
}

- (void)application:(NSApplication *)app openFiles:(NSArray *)filenames {
  for (NSString *filename in filenames) {
    [self application:app openFile:filename];
  }
}

- (void)handleURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)reply {
  (void)reply;
  NSString *url = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
  if (g_app_handler && url) {
    g_app_handler->LoadUrl([url UTF8String]);
  }
}

- (void)applicationDidFinishLaunching:(NSNotification *)note {
  [[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
                                                     andSelector:@selector(handleURLEvent:withReplyEvent:)
                                                   forEventClass:kInternetEventClass
                                                      andEventID:kAEGetURL];
  if (g_gtk_delegate && [g_gtk_delegate respondsToSelector:@selector(applicationDidFinishLaunching:)]) {
    [g_gtk_delegate applicationDidFinishLaunching:note];
  }
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
  (void)sender;
  return MacStartupActions::ShouldTerminateNow() ? NSTerminateNow : NSTerminateCancel;
}

@end

namespace {

StrawberryAppDelegate *g_delegate = nil;

void InstallDelegate() {
  if (g_delegate) {
    return;
  }
  if (!g_gtk_delegate) {
    g_gtk_delegate = [NSApp delegate];
  }
  g_delegate = [[StrawberryAppDelegate alloc] init];
  [NSApp setDelegate:g_delegate];
}

}  // namespace

void MacStartup() {
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  MacOsUtils::SetApplicationName("Strawberry");
  InstallDelegate();
}

void MacSetApplicationHandler(PlatformInterface *handler) {
  g_app_handler = handler;
  InstallDelegate();
}

void MacSetShortcutHandler(GlobalShortcutsBackendMacOs *handler) { g_shortcut_handler = handler; }

void MacSetDockMenu(void *nsmenu) { g_dock_menu = static_cast<NSMenu *>(nsmenu); }

void MacHandleMediaEvent(void *nsevent) {
  NSEvent *event = static_cast<NSEvent *>(nsevent);
  if (!g_shortcut_handler || !event || [event type] != NSEventTypeSystemDefined) {
    return;
  }
  const int data1 = static_cast<int>([event data1]);
  if (!MacStartupActions::ShouldHandleMediaEvent(static_cast<int>([event subtype]), MacStartupActions::MediaKeyFlags(data1))) {
    return;
  }
  g_shortcut_handler->MacMediaKeyPressed(MacStartupActions::MediaKeyCode(data1));
}
#endif
