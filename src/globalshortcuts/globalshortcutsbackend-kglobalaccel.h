#ifndef STRAWBERRY_GLOBALSHORTCUTSBACKEND_KGLOBALACCEL_H
#define STRAWBERRY_GLOBALSHORTCUTSBACKEND_KGLOBALACCEL_H

#include "globalshortcuts/globalshortcutsbackend.h"

#include <gio/gio.h>

#include <string>
#include <vector>

class GlobalShortcut;

class GlobalShortcutsBackendKGlobalAccel : public GlobalShortcutsBackend {
 public:
  explicit GlobalShortcutsBackendKGlobalAccel(GlobalShortcutsManager *manager);
  ~GlobalShortcutsBackendKGlobalAccel() override;

  bool IsAvailable() const override;
  static bool IsKGlobalAccelAvailable();

 protected:
  bool DoRegister() override;
  void DoUnregister() override;

 private:
  bool RegisterShortcut(const GlobalShortcut *shortcut);
  void SubscribeComponent();
  static void OnShortcutPressed(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
                                const gchar *signal_name, GVariant *parameters, gpointer data);

  GDBusProxy *interface_ = nullptr;
  guint signal_id_ = 0;
  std::vector<std::string> registered_ids_;
};

class GlobalShortcutsBackendGnome : public GlobalShortcutsBackend {
 public:
  explicit GlobalShortcutsBackendGnome(GlobalShortcutsManager *manager);
  ~GlobalShortcutsBackendGnome() override;

  bool IsAvailable() const override;

 protected:
  bool DoRegister() override;
  void DoUnregister() override;

 private:
  static void OnMediaKey(GDBusProxy *proxy, const char *sender, const char *signal, GVariant *parameters, gpointer data);

  GDBusProxy *media_keys_ = nullptr;
};

#endif
