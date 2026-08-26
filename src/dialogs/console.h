#ifndef STRAWBERRY_CONSOLE_H
#define STRAWBERRY_CONSOLE_H

#include <gtk/gtk.h>

class Database;

class Console {
 public:
  static void Show(GtkWindow *parent, Database *database = nullptr);
};

#endif
