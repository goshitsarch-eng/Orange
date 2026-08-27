#ifndef STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGET_H
#define STRAWBERRY_SMARTPLAYLISTSEARCHTERMWIDGET_H

#include "smartplaylists/smartplaylist.h"

#include <gtk/gtk.h>

#include <functional>
#include <vector>

class SmartPlaylistSearchTermWidget {
 public:
  using ChangedCallback = std::function<void()>;
  using ClickedCallback = std::function<void()>;
  using RemoveCallback = std::function<void()>;

  SmartPlaylistSearchTermWidget();

  GtkWidget *widget() const { return widget_; }
  SmartPlaylistTerm Term() const;
  void SetTerm(const SmartPlaylistTerm &term);
  bool IsEmpty() const;
  bool active() const { return active_; }
  void SetActive(bool active);
  void SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }
  void SetClickedCallback(ClickedCallback callback) { clicked_ = std::move(callback); }
  void SetRemoveCallback(RemoveCallback callback) { removed_ = std::move(callback); }

 private:
  void RebuildOps();
  void RebuildValue();
  void ConnectValueSignals();
  void EmitChanged();
  std::string CurrentValue() const;
  void SetCurrentValue(const std::string &value);
  void ApplyActive();

  GtkWidget *widget_ = nullptr;
  GtkWidget *row_ = nullptr;
  GtkWidget *field_ = nullptr;
  GtkWidget *op_ = nullptr;
  GtkWidget *value_ = nullptr;
  GtkWidget *remove_ = nullptr;
  GtkWidget *overlay_ = nullptr;
  std::vector<SmartPlaylistOp> current_ops_;
  bool updating_ = false;
  bool active_ = true;
  ChangedCallback changed_;
  ClickedCallback clicked_;
  RemoveCallback removed_;
};

#endif
