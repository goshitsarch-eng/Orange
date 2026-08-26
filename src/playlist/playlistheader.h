#ifndef STRAWBERRY_PLAYLISTHEADER_H
#define STRAWBERRY_PLAYLISTHEADER_H

#include "playlist/playlistdelegates.h"

#include <gtk/gtk.h>

#include <functional>

class PlaylistHeader {
 public:
  using SortCallback = std::function<void(PlaylistColumn, PlaylistSortOrder)>;
  using LayoutChangedCallback = std::function<void()>;

  PlaylistHeader();

  GtkWidget *widget() const { return widget_; }
  void SetSortCallback(SortCallback callback) { sort_ = std::move(callback); }
  void SetLayoutChangedCallback(LayoutChangedCallback callback) { layout_changed_ = std::move(callback); }
  void Rebuild();

 private:
  void ShowMenu(PlaylistColumn column);
  void NotifyLayoutChanged();
  PlaylistColumn ColumnAtX(double x) const;

  GtkWidget *widget_ = nullptr;
  SortCallback sort_;
  LayoutChangedCallback layout_changed_;
};

#endif
