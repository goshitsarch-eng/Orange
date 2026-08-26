#ifndef STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGET_H
#define STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGET_H

#include "smartplaylists/smartplaylist.h"

#include <gtk/gtk.h>

#include <vector>

class SmartPlaylistSearchTermWidget {
 public:
  SmartPlaylistSearchTermWidget();

  GtkWidget *widget() const { return widget_; }
  SmartPlaylistTerm Term() const;
  void SetTerm(const SmartPlaylistTerm &term);
  bool IsEmpty() const;

 private:
  void RebuildOps();
  void RebuildValue();
  std::string CurrentValue() const;
  void SetCurrentValue(const std::string &value);

  GtkWidget *widget_ = nullptr;
  GtkWidget *field_ = nullptr;
  GtkWidget *op_ = nullptr;
  GtkWidget *value_ = nullptr;
  std::vector<SmartPlaylistOp> current_ops_;
  bool updating_ = false;
};

#endif
