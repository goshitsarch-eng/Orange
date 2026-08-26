#ifndef STRAWBERRY_GLOBALSHORTCUTSBACKEND_KGLOBALACCEL_H
#define STRAWBERRY_GLOBALSHORTCUTSBACKEND_KGLOBALACCEL_H

#include "globalshortcuts/globalshortcutsbackend.h"

#include <gio/gio.h>

class GlobalShortcutsBackendKGlobalAccel : public GlobalShortcutsBackend {
 public:
  explicit GlobalShortcutsBackendKGlobalAccel(GlobalShortcutsManager *manager);
  ~GlobalShortcutsBackendKGlobalAccel() override;

  bool IsAvailable() const override;

 protected:
  bool DoRegister() override;
  void DoUnregister() override;

 private:
  static void OnMediaKey(GDBusProxy *proxy, const char *sender, const char *signal, GVariant *parameters, gpointer data);

  GDBusProxy *media_keys_ = nullptr;
};

#endif
