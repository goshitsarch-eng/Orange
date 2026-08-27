#ifndef STRAWBERRY_GLOBALSHORTCUTSBACKEND_MACOS_H
#define STRAWBERRY_GLOBALSHORTCUTSBACKEND_MACOS_H

#include "globalshortcuts/globalshortcutsbackend.h"

class GlobalShortcutsBackendMacOs : public GlobalShortcutsBackend {
 public:
  explicit GlobalShortcutsBackendMacOs(GlobalShortcutsManager *manager);
  ~GlobalShortcutsBackendMacOs() override;

  bool IsAvailable() const override;
  static bool IsAccessibilityEnabled();
  static void ShowAccessibilityDialog();

 protected:
  bool DoRegister() override;
  void DoUnregister() override;

 private:
  bool HandleAccel(const std::string &accel);
  void HandleMediaKey(int nx_key);

  void *global_monitor_ = nullptr;
  void *local_monitor_ = nullptr;
};

#endif
