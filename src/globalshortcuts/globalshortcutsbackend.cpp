#include "globalshortcuts/globalshortcutsbackend.h"

GlobalShortcutsBackend::GlobalShortcutsBackend(GlobalShortcutsManager *manager, Type type) : manager_(manager), type_(type) {}

std::string GlobalShortcutsBackend::name() const {
  switch (type_) {
    case Type::KGlobalAccel:
      return "KGlobalAccel";
    case Type::Gnome:
      return "Gnome";
    case Type::X11:
      return "X11";
    case Type::Portal:
      return "Portal";
    case Type::macOS:
      return "macOS";
    case Type::Win:
      return "Win";
    case Type::None:
    default:
      return "None";
  }
}

bool GlobalShortcutsBackend::Register() {
  if (active_) {
    return true;
  }
  active_ = DoRegister();
  return active_;
}

void GlobalShortcutsBackend::Unregister() {
  if (!active_) {
    return;
  }
  DoUnregister();
  active_ = false;
}
