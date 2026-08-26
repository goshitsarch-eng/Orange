#ifndef STRAWBERRY_ORGANIZEERRORDIALOG_H
#define STRAWBERRY_ORGANIZEERRORDIALOG_H

#include "organize/organize.h"

#include <gtk/gtk.h>

class OrganizeErrorDialog {
 public:
  static void Show(GtkWindow *parent, const std::vector<Organize::Error> &errors);
};

#endif
