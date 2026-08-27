#ifndef STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGET_H
#define STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGET_H

#include "smartplaylists/smartplaylist.h"

#include <gtk/gtk.h>

#include <functional>
#include <vector>

class SmartPlaylistSearchTermWidget {
 public:
  using ChangedCallback = std::function<void()>;

  SmartPlaylistSearchTermWidget();

  GtkWidget *widget() const { return widget_; }
  SmartPlaylistTerm Term() const;
  void SetTerm(const SmartPlaylistTerm &term);
  bool IsEmpty() const;
  void SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }

 private:
  void RebuildOps();
  void RebuildValue();
  void ConnectValueSignals();
  void EmitChanged();
  std::string CurrentValue() const;
  void SetCurrentValue(const std::string &value);

  GtkWidget *widget_ = nullptr;
  GtkWidget *field_ = nullptr;
  GtkWidget *op_ = nullptr;
  GtkWidget *value_ = nullptr;
  std::vector<SmartPlaylistOp> current_ops_;
  bool updating_ = false;
  ChangedCallback changed_;
};

#endif
