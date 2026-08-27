#ifndef STRAWBERRY_GLOBALSHORTCUTSBACKEND_WIN_H
#define STRAWBERRY_GLOBALSHORTCUTSBACKEND_WIN_H

#include "globalshortcuts/globalshortcutsbackend.h"

#include <map>
#include <string>

class GlobalShortcutsBackendWin : public GlobalShortcutsBackend {
 public:
  explicit GlobalShortcutsBackendWin(GlobalShortcutsManager *manager);
  ~GlobalShortcutsBackendWin() override;

  bool IsAvailable() const override;

 protected:
  bool DoRegister() override;
  void DoUnregister() override;

 private:
  bool AddShortcut(const std::string &id, const std::string &key);
  void RemoveShortcut(const std::string &id);

  std::map<std::string, int> ids_;
};

#endif
