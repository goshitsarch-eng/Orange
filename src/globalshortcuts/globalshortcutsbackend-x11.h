#ifndef STRAWBERRY_GLOBALSHORTCUTSBACKEND_X11_H
#define STRAWBERRY_GLOBALSHORTCUTSBACKEND_X11_H

#include "config.h"
#include "globalshortcuts/globalshortcutsbackend.h"

#ifdef HAVE_X11
#include <glib.h>
#endif

#include <vector>

class GlobalShortcutsBackendX11 : public GlobalShortcutsBackend {
 public:
  explicit GlobalShortcutsBackendX11(GlobalShortcutsManager *manager);
  ~GlobalShortcutsBackendX11() override;

  bool IsAvailable() const override;

 protected:
  bool DoRegister() override;
  void DoUnregister() override;

 private:
#ifdef HAVE_X11
  static gboolean OnX11Fd(gint fd, GIOCondition condition, gpointer data);
  void HandleX11Event();
#endif

  void *x11_display_ = nullptr;
  unsigned int x11_watch_id_ = 0;
  std::vector<unsigned int> x11_codes_;
};

#endif
