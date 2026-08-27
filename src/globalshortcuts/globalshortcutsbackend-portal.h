#ifndef STRAWBERRY_GLOBALSHORTCUTSBACKEND_PORTAL_H
#define STRAWBERRY_GLOBALSHORTCUTSBACKEND_PORTAL_H

#include "globalshortcuts/globalshortcutsbackend.h"

#include <gio/gio.h>

#include <string>

class GlobalShortcutsBackendPortal : public GlobalShortcutsBackend {
 public:
  explicit GlobalShortcutsBackendPortal(GlobalShortcutsManager *manager);
  ~GlobalShortcutsBackendPortal() override;

  bool IsAvailable() const override;
  static bool IsPortalAvailable();

 protected:
  bool DoRegister() override;
  void DoUnregister() override;

 private:
  void BindSession(const std::string &session_path);
  static void OnActivated(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
                          const gchar *signal_name, GVariant *parameters, gpointer data);

  GDBusConnection *connection_ = nullptr;
  guint activated_id_ = 0;
  guint response_id_ = 0;
  std::string session_path_;
};

#endif
