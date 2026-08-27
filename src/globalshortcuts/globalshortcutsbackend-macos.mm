#include "globalshortcuts/globalshortcutsbackend-macos.h"

#include "core/logging.h"
#include "globalshortcuts/globalshortcut.h"
#include "globalshortcuts/globalshortcuts.h"

#ifdef __APPLE__
#include <Carbon/Carbon.h>
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

bool GlobalShortcutsBackendMacOs::DoRegister() {
#ifdef __APPLE__
  LogDebug("Registering macOS global shortcuts");
  return manager_ != nullptr;
#else
  return false;
#endif
}

void GlobalShortcutsBackendMacOs::DoUnregister() {
#ifdef __APPLE__
  LogDebug("Unregistering macOS global shortcuts");
#endif
}
