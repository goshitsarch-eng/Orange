#ifndef STRAWBERRY_EQUALIZERDIALOG_H
#define STRAWBERRY_EQUALIZERDIALOG_H

#include <gtk/gtk.h>

class Application;
class Equalizer;

class EqualizerDialog {
 public:
  static void Show(GtkWindow *parent, Equalizer *equalizer, Application *app = nullptr);
};

#endif
