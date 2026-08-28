#ifndef STRAWBERRY_PLAYLISTHEADER_H
#define STRAWBERRY_PLAYLISTHEADER_H

#include "playlist/playlistdelegates.h"

#include <gtk/gtk.h>

#include <functional>

class PlaylistHeader {
 public:
  using SortCallback = std::function<void(PlaylistColumn, PlaylistSortOrder)>;
  using LayoutChangedCallback = std::function<void()>;
  using WidthsChangedCallback = std::function<void()>;

  PlaylistHeader();

  GtkWidget *widget() const { return widget_; }
  void SetSortCallback(SortCallback callback) { sort_ = std::move(callback); }
  void SetLayoutChangedCallback(LayoutChangedCallback callback) { layout_changed_ = std::move(callback); }
  void SetWidthsChangedCallback(WidthsChangedCallback callback) { widths_changed_ = std::move(callback); }
  void SetSortState(PlaylistColumn column, bool descending);
  void SetViewportWidth(int width);
  void ApplyWidths();
  void Rebuild();
  gboolean OnKeyPressed(guint keyval, GdkModifierType state);

 private:
  void ShowMenu(PlaylistColumn column);
  void NotifyLayoutChanged();
  PlaylistColumn ColumnAtX(double x) const;
  PlaylistColumn ResizeColumnAtX(double x) const;
  PlaylistColumn NextVisible(PlaylistColumn column) const;
  void UpdateResizeCursor(double x);
  void OnDragBegin(double x);
  void OnDragUpdate(double offset_x);
  void OnDragEnd();
  void ReorderButtons();
  void NotifyWidthsChanged();
  bool DragActive() const;

  GtkWidget *widget_ = nullptr;
  SortCallback sort_;
  LayoutChangedCallback layout_changed_;
  WidthsChangedCallback widths_changed_;
  PlaylistColumn sort_column_ = PlaylistColumn::Count;
  bool sort_descending_ = false;
  int viewport_width_ = 0;
  PlaylistColumn resize_column_ = PlaylistColumn::Count;
  PlaylistColumn resize_next_ = PlaylistColumn::Count;
  int resize_left_start_ = 0;
  int resize_right_start_ = 0;
  PlaylistColumn reorder_column_ = PlaylistColumn::Count;
  PlaylistColumn reorder_last_hover_ = PlaylistColumn::Count;
  double drag_start_x_ = 0;
};

#endif
