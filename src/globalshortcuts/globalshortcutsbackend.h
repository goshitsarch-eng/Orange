#ifndef STRAWBERRY_GLOBALSHORTCUTSBACKEND_H
#define STRAWBERRY_GLOBALSHORTCUTSBACKEND_H

#include <string>

class GlobalShortcutsManager;

class GlobalShortcutsBackend {
 public:
  enum class Type { None = 0, KGlobalAccel, Gnome, X11, Portal, macOS, Win };

  GlobalShortcutsBackend(GlobalShortcutsManager *manager, Type type);
  virtual ~GlobalShortcutsBackend() = default;

  Type type() const { return type_; }
  std::string name() const;
  virtual bool IsAvailable() const = 0;
  bool Register();
  void Unregister();
  bool is_active() const { return active_; }

 protected:
  virtual bool DoRegister() = 0;
  virtual void DoUnregister() = 0;

  GlobalShortcutsManager *manager_ = nullptr;
  Type type_ = Type::None;
  bool active_ = false;
};

#endif
