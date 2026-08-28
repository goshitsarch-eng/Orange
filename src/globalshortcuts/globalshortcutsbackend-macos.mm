#include "globalshortcuts/globalshortcutsbackend-macos.h"

#include "core/logging.h"
#include "globalshortcuts/globalshortcut.h"
#include "globalshortcuts/globalshortcuts.h"
#include "globalshortcuts/keymapper_macos.h"
#include "globalshortcuts/macosaccessibility.h"
#ifdef __APPLE__
#include "core/mac_startup.h"
#endif

#ifdef __APPLE__
#include <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/hidsystem/ev_keymap.h>
#endif

GlobalShortcutsBackendMacOs::GlobalShortcutsBackendMacOs(GlobalShortcutsManager *manager)
    : GlobalShortcutsBackend(manager, GlobalShortcutsBackend::Type::macOS) {}

GlobalShortcutsBackendMacOs::~GlobalShortcutsBackendMacOs() { DoUnregister(); }

bool GlobalShortcutsBackendMacOs::IsAvailable() const {
#ifdef __APPLE__
  return true;
#else
  return false;
#endif
}

bool GlobalShortcutsBackendMacOs::IsAccessibilityEnabled() {
#ifdef __APPLE__
  const id prompt = MacOsAccessibility::PromptWhenCheckingTrust() ? @YES : @NO;
  NSDictionary *options = @{(__bridge id)kAXTrustedCheckOptionPrompt : prompt};
  return AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
#else
  return false;
#endif
}

void GlobalShortcutsBackendMacOs::ShowAccessibilityDialog() {
#ifdef __APPLE__
  NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:MacOsAccessibility::PreferencesUrl()]];
  if (url) {
    [[NSWorkspace sharedWorkspace] openURL:url];
  }
#endif
}

bool GlobalShortcutsBackendMacOs::HandleAccel(const std::string &accel) {
  if (!manager_ || accel.empty()) {
    return false;
  }
  for (const auto &shortcut : manager_->shortcuts()) {
    const std::string bound = shortcut->key().empty() ? shortcut->default_key() : shortcut->key();
    if (KeyMapperMacOs::KeysMatch(accel, bound)) {
      manager_->Emit(shortcut->id());
      return true;
    }
  }
  const char *id = KeyMapperMacOs::ShortcutIdFromKey(accel);
  if (id && *id) {
    manager_->Emit(id);
    return true;
  }
  return false;
}

void GlobalShortcutsBackendMacOs::MacMediaKeyPressed(int nx_key) { HandleMediaKey(nx_key); }

void GlobalShortcutsBackendMacOs::HandleMediaKey(int nx_key) {
  const char *id = KeyMapperMacOs::IdFromMediaKey(nx_key);
  if (id && *id && manager_) {
    manager_->Emit(id);
  }
}

bool GlobalShortcutsBackendMacOs::DoRegister() {
#ifdef __APPLE__
  if (!manager_) {
    return false;
  }
  LogDebug("Registering macOS global shortcuts");

  auto handle_event = [](NSEvent *event, GlobalShortcutsBackendMacOs *self) -> BOOL {
    if ([event type] == NSEventTypeSystemDefined && KeyMapperMacOs::IsAuxControlEvent(static_cast<int>([event subtype]))) {
      const int data1 = static_cast<int>([event data1]);
      if (KeyMapperMacOs::IsMediaKeyDown(data1)) {
        self->HandleMediaKey(KeyMapperMacOs::MediaKeyFromData1(data1));
        return YES;
      }
      return NO;
    }
    if ([event type] != NSEventTypeKeyDown) {
      return NO;
    }
    unsigned modifiers = 0;
    const NSEventModifierFlags flags = [event modifierFlags];
    if (flags & NSEventModifierFlagControl) {
      modifiers |= KeyMapperMacOs::kControl;
    }
    if (flags & NSEventModifierFlagOption) {
      modifiers |= KeyMapperMacOs::kOption;
    }
    if (flags & NSEventModifierFlagShift) {
      modifiers |= KeyMapperMacOs::kShift;
    }
    if (flags & NSEventModifierFlagCommand) {
      modifiers |= KeyMapperMacOs::kCommand;
    }
    NSString *chars = [event charactersIgnoringModifiers];
    const std::string key = chars && [chars length] ? [[chars uppercaseString] UTF8String] : "";
    return self->HandleAccel(KeyMapperMacOs::AcceleratorFromEvent(modifiers, key)) ? YES : NO;
  };

  global_monitor_ = (void *)[NSEvent addGlobalMonitorForEventsMatchingMask:(NSEventMaskKeyDown | NSEventMaskSystemDefined)
                                                                 handler:^(NSEvent *event) {
                                                                   handle_event(event, this);
                                                                 }];
  local_monitor_ = (void *)[NSEvent addLocalMonitorForEventsMatchingMask:(NSEventMaskKeyDown | NSEventMaskSystemDefined)
                                                               handler:^(NSEvent *event) {
                                                                 return handle_event(event, this) ? nil : event;
                                                               }];
  MacSetShortcutHandler(this);
  return true;
#else
  return false;
#endif
}

void GlobalShortcutsBackendMacOs::DoUnregister() {
#ifdef __APPLE__
  LogDebug("Unregistering macOS global shortcuts");
  if (global_monitor_) {
    [NSEvent removeMonitor:(id)global_monitor_];
    global_monitor_ = nullptr;
  }
  if (local_monitor_) {
    [NSEvent removeMonitor:(id)local_monitor_];
    local_monitor_ = nullptr;
  }
  MacSetShortcutHandler(nullptr);
#endif
}
