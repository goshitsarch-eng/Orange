#ifndef STRAWBERRY_TRANSCODELOGDIALOG_H
#define STRAWBERRY_TRANSCODELOGDIALOG_H

#include <gtk/gtk.h>

#include <string>
#include <vector>

class TranscodeLogDialog {
 public:
  static void Show(GtkWindow *parent, std::vector<std::string> *lines, GtkWidget **view_slot, GtkWidget *preview = nullptr);
};

#endif
