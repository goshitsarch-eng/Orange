#ifndef STRAWBERRY_GLOBALSHORTCUTSBACKEND_MACOS_H
#define STRAWBERRY_GLOBALSHORTCUTSBACKEND_MACOS_H

#include "globalshortcuts/globalshortcutsbackend.h"

class GlobalShortcutsBackendMacOs : public GlobalShortcutsBackend {
 public:
  explicit GlobalShortcutsBackendMacOs(GlobalShortcutsManager *manager);
  ~GlobalShortcutsBackendMacOs() override;

  bool IsAvailable() const override;

 protected:
  bool DoRegister() override;
  void DoUnregister() override;
};

#endif
