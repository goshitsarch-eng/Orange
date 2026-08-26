#ifndef STRAWBERRY_GLOBALSHORTCUTSBACKEND_X11_H
#define STRAWBERRY_GLOBALSHORTCUTSBACKEND_X11_H

#include "config.h"
#include "globalshortcuts/globalshortcutsbackend.h"

#ifdef HAVE_X11
#include <glib.h>
#endif

#include <string>
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
  void GrabKey(unsigned int code, unsigned int modifiers, bool any_modifier, const std::string &id);
#endif

  struct Binding {
    unsigned int code = 0;
    unsigned int modifiers = 0;
    bool any_modifier = false;
    std::string id;
  };

  struct Grab {
    unsigned int code = 0;
    unsigned int modifiers = 0;
  };

  void *x11_display_ = nullptr;
  unsigned int x11_watch_id_ = 0;
  std::vector<Binding> bindings_;
  std::vector<Grab> grabs_;
};

#endif
