#ifndef STRAWBERRY_TRANSCODEDIALOG_H
#define STRAWBERRY_TRANSCODEDIALOG_H

#include <gtk/gtk.h>

class Application;

class TranscodeDialog {
 public:
  static void Show(GtkWindow *parent, Application *app);
};

#endif
