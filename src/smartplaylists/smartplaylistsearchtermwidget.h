#ifndef STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGET_H
#define STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGET_H

#include "smartplaylists/smartplaylist.h"

#include <gtk/gtk.h>

class SmartPlaylistSearchTermWidget {
 public:
  SmartPlaylistSearchTermWidget();

  GtkWidget *widget() const { return widget_; }
  SmartPlaylistTerm Term() const;
  void SetTerm(const SmartPlaylistTerm &term);
  bool IsEmpty() const;

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *field_ = nullptr;
  GtkWidget *op_ = nullptr;
  GtkWidget *value_ = nullptr;
};

#endif
