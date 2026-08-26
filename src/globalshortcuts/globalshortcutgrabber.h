#ifndef STRAWBERRY_GLOBALSHORTCUTGRABBER_H
#define STRAWBERRY_GLOBALSHORTCUTGRABBER_H

#include <functional>
#include <string>

#include <gtk/gtk.h>

class GlobalShortcutGrabber {
 public:
  static void Show(GtkWindow *parent, const std::function<void(const std::string &)> &callback);
};

#endif
