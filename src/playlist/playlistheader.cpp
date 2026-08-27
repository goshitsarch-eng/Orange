#include "config.h"

#include "playlist/playlistheader.h"

#include "playlist/playlistcolumnlayout.h"
#include "playlist/playlistcolumnwidths.h"
#include "playlist/playlistheaderreorder.h"
#include "playlist/playlistheadersort.h"
#include "playlist/playlistmoodcolumn.h"
#include "translations/translations.h"

#include <algorithm>
#include <string>
#include <vector>

PlaylistHeader::PlaylistHeader() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(widget_, "toolbar");
  gtk_widget_add_css_class(widget_, "strawberry-playlist-buttons");
  gtk_widget_set_focusable(widget_, TRUE);
  GtkGesture *gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(gesture));
  g_signal_connect(gesture, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble x, gdouble, gpointer data) {
                     auto *self = static_cast<PlaylistHeader *>(data);
                     self->ShowMenu(self->ColumnAtX(x));
                   }),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(widget_, keys);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     return static_cast<PlaylistHeader *>(data)->OnKeyPressed(keyval, state);
                   })),
                   this);
  GtkEventController *motion = gtk_event_controller_motion_new();
  gtk_widget_add_controller(widget_, motion);
  g_signal_connect(motion, "motion", G_CALLBACK(+[](GtkEventControllerMotion *, gdouble x, gdouble, gpointer data) {
                     static_cast<PlaylistHeader *>(data)->UpdateResizeCursor(x);
                   }),
                   this);
  GtkGesture *drag = gtk_gesture_drag_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(drag));
  g_signal_connect(drag, "drag-begin", G_CALLBACK(+[](GtkGestureDrag *g, gdouble x, gdouble, gpointer data) {
                     auto *self = static_cast<PlaylistHeader *>(data);
                     self->OnDragBegin(x);
                     gtk_gesture_set_state(GTK_GESTURE(g), self->DragActive() ? GTK_EVENT_SEQUENCE_CLAIMED : GTK_EVENT_SEQUENCE_DENIED);
                   }),
                   this);
  g_signal_connect(drag, "drag-update", G_CALLBACK(+[](GtkGestureDrag *, gdouble offset_x, gdouble, gpointer data) {
                     static_cast<PlaylistHeader *>(data)->OnDragUpdate(offset_x);
                   }),
                   this);
  g_signal_connect(drag, "drag-end", G_CALLBACK(+[](GtkGestureDrag *, gdouble, gdouble, gpointer data) {
                     static_cast<PlaylistHeader *>(data)->OnDragEnd();
                   }),
                   this);
}

gboolean PlaylistHeader::OnKeyPressed(guint keyval, GdkModifierType state) {
  if (!PlaylistHeaderSort::IsKeyboardTrigger(keyval, static_cast<unsigned>(state))) {
    return FALSE;
  }
  if (PlaylistHeaderSort::ShouldShowMenu()) {
    const auto visible = PlaylistColumnLayout::Visible();
    const PlaylistColumn first = visible.empty() ? PlaylistColumn::Title : visible.front();
    ShowMenu(PlaylistHeaderSort::ColumnForMenu(true, ColumnAtX(0), first));
  }
  return TRUE;
}

void PlaylistHeader::SetSortState(PlaylistColumn column, bool descending) {
  sort_column_ = column;
  sort_descending_ = descending;
}

void PlaylistHeader::SetViewportWidth(int width) { viewport_width_ = std::max(0, width); }

void PlaylistHeader::ApplyWidths() {
  const int total = viewport_width_ > 0 ? viewport_width_ : gtk_widget_get_width(widget_);
  int sum = 0;
  for (GtkWidget *child = gtk_widget_get_first_child(widget_); child; child = gtk_widget_get_next_sibling(child)) {
    const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "column"));
    if (stored <= 0) {
      continue;
    }
    const PlaylistColumn column = static_cast<PlaylistColumn>(stored - 1);
    const int width = PlaylistColumnLayout::PixelWidth(column, total);
    gtk_widget_set_hexpand(child, FALSE);
    gtk_widget_set_size_request(child, width, -1);
    sum += width;
  }
  if (sum > 0) {
    gtk_widget_set_size_request(widget_, PlaylistColumnLayout::StretchEnabled() ? std::max(total, sum) : sum, -1);
  }
}

void PlaylistHeader::NotifyWidthsChanged() {
  if (widths_changed_) {
    widths_changed_();
  }
}

PlaylistColumn PlaylistHeader::NextVisible(PlaylistColumn column) const {
  const auto columns = PlaylistColumnLayout::Visible();
  for (size_t i = 0; i < columns.size(); ++i) {
    if (columns[i] == column && i + 1 < columns.size()) {
      return columns[i + 1];
    }
  }
  return PlaylistColumn::Count;
}

PlaylistColumn PlaylistHeader::ResizeColumnAtX(double x) const {
  for (GtkWidget *child = gtk_widget_get_first_child(widget_); child; child = gtk_widget_get_next_sibling(child)) {
    const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "column"));
    if (stored <= 0) {
      continue;
    }
    graphene_rect_t bounds{};
    if (!gtk_widget_compute_bounds(child, widget_, &bounds)) {
      continue;
    }
    if (!PlaylistColumnWidths::OnResizeHandleAbsolute(x, bounds.origin.x, bounds.size.width)) {
      continue;
    }
    const PlaylistColumn column = static_cast<PlaylistColumn>(stored - 1);
    if (NextVisible(column) != PlaylistColumn::Count) {
      return column;
    }
  }
  return PlaylistColumn::Count;
}

void PlaylistHeader::UpdateResizeCursor(double x) {
  if (ResizeColumnAtX(x) != PlaylistColumn::Count) {
    gtk_widget_set_cursor_from_name(widget_, "col-resize");
    return;
  }
  gtk_widget_set_cursor_from_name(widget_, ColumnAtX(x) == PlaylistColumn::Count ? nullptr : "grab");
}

bool PlaylistHeader::DragActive() const {
  return resize_column_ != PlaylistColumn::Count || reorder_column_ != PlaylistColumn::Count;
}

void PlaylistHeader::ReorderButtons() {
  std::vector<GtkWidget *> buttons;
  for (GtkWidget *child = gtk_widget_get_first_child(widget_); child;) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "column")) > 0) {
      g_object_ref(child);
      gtk_widget_unparent(child);
      buttons.push_back(child);
    }
    child = next;
  }
  for (PlaylistColumn column : PlaylistColumnLayout::Visible()) {
    for (GtkWidget *button : buttons) {
      if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "column")) - 1 == static_cast<int>(column)) {
        gtk_box_append(GTK_BOX(widget_), button);
        g_object_unref(button);
        break;
      }
    }
  }
}

void PlaylistHeader::OnDragBegin(double x) {
  drag_start_x_ = x;
  resize_column_ = ResizeColumnAtX(x);
  resize_next_ = NextVisible(resize_column_);
  reorder_column_ = PlaylistColumn::Count;
  reorder_last_hover_ = PlaylistColumn::Count;
  if (resize_column_ != PlaylistColumn::Count && resize_next_ != PlaylistColumn::Count) {
    const int total = viewport_width_ > 0 ? viewport_width_ : gtk_widget_get_width(widget_);
    resize_left_start_ = PlaylistColumnLayout::PixelWidth(resize_column_, total);
    resize_right_start_ = PlaylistColumnLayout::PixelWidth(resize_next_, total);
    return;
  }
  resize_column_ = PlaylistColumn::Count;
  resize_next_ = PlaylistColumn::Count;
  reorder_column_ = ColumnAtX(x);
  reorder_last_hover_ = reorder_column_;
}

void PlaylistHeader::OnDragEnd() {
  if (reorder_column_ != PlaylistColumn::Count) {
    NotifyLayoutChanged();
  }
  resize_column_ = PlaylistColumn::Count;
  resize_next_ = PlaylistColumn::Count;
  reorder_column_ = PlaylistColumn::Count;
  reorder_last_hover_ = PlaylistColumn::Count;
}

void PlaylistHeader::OnDragUpdate(double offset_x) {
  if (reorder_column_ != PlaylistColumn::Count) {
    const PlaylistColumn hover = ColumnAtX(drag_start_x_ + offset_x);
    if (!PlaylistHeaderReorder::ShouldApplyReorder(reorder_column_, hover) || hover == reorder_last_hover_) {
      return;
    }
    reorder_last_hover_ = hover;
    PlaylistColumnLayout::MoveTo(reorder_column_, PlaylistHeaderReorder::VisualIndex(PlaylistColumnLayout::Visible(), hover));
    ReorderButtons();
    ApplyWidths();
    return;
  }
  if (resize_column_ == PlaylistColumn::Count || resize_next_ == PlaylistColumn::Count) {
    return;
  }
  const int left_new = resize_left_start_ + static_cast<int>(offset_x);
  int right_new = resize_right_start_;
  if (!PlaylistColumnWidths::NeighborResize(resize_left_start_, left_new, resize_right_start_, &right_new)) {
    return;
  }
  const int total = viewport_width_ > 0 ? viewport_width_ : gtk_widget_get_width(widget_);
  PlaylistColumnLayout::ResizePair(resize_column_, left_new, resize_next_, right_new, total);
  ApplyWidths();
  NotifyWidthsChanged();
}

void PlaylistHeader::Rebuild() {
  GtkWidget *child = gtk_widget_get_first_child(widget_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_widget_unparent(child);
    child = next;
  }
  for (PlaylistColumn column : PlaylistColumnLayout::Visible()) {
    const std::string title =
        PlaylistHeaderSort::LabelForColumn(PlaylistDelegates::ColumnTitle(column), column, sort_column_, sort_descending_);
    GtkWidget *button = gtk_button_new_with_label(title.c_str());
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_hexpand(button, FALSE);
    gtk_widget_set_size_request(button, PlaylistColumnLayout::PixelWidth(column, viewport_width_), -1);
    GtkWidget *label = gtk_widget_get_first_child(button);
    if (GTK_IS_LABEL(label)) {
      gtk_label_set_xalign(GTK_LABEL(label), PlaylistColumnLayout::XAlign(column));
    }
    g_object_set_data(G_OBJECT(button), "column", GINT_TO_POINTER(static_cast<int>(column) + 1));
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<PlaylistHeader *>(data);
                       if (self->sort_) {
                         self->sort_(static_cast<PlaylistColumn>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "column")) - 1),
                                     PlaylistSortOrder::Toggle);
                       }
                     }),
                     this);
    gtk_box_append(GTK_BOX(widget_), button);
  }
  ApplyWidths();
}

PlaylistColumn PlaylistHeader::ColumnAtX(double x) const {
  for (GtkWidget *child = gtk_widget_get_first_child(widget_); child; child = gtk_widget_get_next_sibling(child)) {
    const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "column"));
    if (stored <= 0) {
      continue;
    }
    graphene_rect_t bounds{};
    if (!gtk_widget_compute_bounds(child, widget_, &bounds)) {
      continue;
    }
    if (x >= bounds.origin.x && x < bounds.origin.x + bounds.size.width) {
      return static_cast<PlaylistColumn>(stored - 1);
    }
  }
  const auto columns = PlaylistColumnLayout::Visible();
  return columns.empty() ? PlaylistColumn::Title : columns.front();
}

void PlaylistHeader::NotifyLayoutChanged() {
  if (!layout_changed_) {
    Rebuild();
    return;
  }
  auto *cb = new LayoutChangedCallback(layout_changed_);
  g_idle_add(+[](gpointer data) -> gboolean {
    auto *fn = static_cast<LayoutChangedCallback *>(data);
    if (fn && *fn) {
      (*fn)();
    }
    delete fn;
    return G_SOURCE_REMOVE;
  },
             cb);
}

void PlaylistHeader::ShowMenu(PlaylistColumn column) {
  GtkWidget *popover = gtk_popover_new();
  gtk_widget_set_parent(popover, widget_);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_margin_start(box, 8);
  gtk_widget_set_margin_end(box, 8);
  gtk_widget_set_margin_top(box, 8);
  gtk_widget_set_margin_bottom(box, 8);

  auto add_button = [&](const char *label, const std::function<void()> &action) {
    GtkWidget *button = gtk_button_new_with_label(label);
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_halign(button, GTK_ALIGN_FILL);
    auto *cb = new std::function<void()>(action);
    g_object_set_data_full(G_OBJECT(button), "action", cb, [](gpointer p) { delete static_cast<std::function<void()> *>(p); });
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *fn = static_cast<std::function<void()> *>(g_object_get_data(G_OBJECT(btn), "action"));
                       gtk_popover_popdown(GTK_POPOVER(data));
                       if (fn && *fn) {
                         (*fn)();
                       }
                     }),
                     popover);
    gtk_box_append(GTK_BOX(box), button);
  };

  if (PlaylistColumnLayout::CanHide()) {
    const std::string hide_label = std::string(Translations::CStr("Hide")) + " " + PlaylistDelegates::ColumnTitle(column);
    add_button(hide_label.c_str(), [this, column]() {
      PlaylistColumnLayout::Hide(column);
      NotifyLayoutChanged();
    });
  }

  GtkWidget *stretch = gtk_check_button_new_with_label(Translations::CStr("Stretch columns to fit window"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(stretch), PlaylistColumnLayout::StretchEnabled());
  g_signal_connect(stretch, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                     PlaylistColumnLayout::SetStretchEnabled(gtk_check_button_get_active(button));
                     static_cast<PlaylistHeader *>(data)->NotifyLayoutChanged();
                   }),
                   this);
  gtk_box_append(GTK_BOX(box), stretch);

  add_button(Translations::CStr("Reset columns to default"), [this]() {
    PlaylistColumnLayout::Reset();
    NotifyLayoutChanged();
  });

  if (column == PlaylistColumn::Rating) {
    GtkWidget *lock = gtk_check_button_new_with_label(Translations::CStr("Lock rating"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(lock), PlaylistColumnLayout::RatingLocked());
    g_signal_connect(lock, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer) {
                       PlaylistColumnLayout::SetRatingLocked(gtk_check_button_get_active(button));
                     }),
                     nullptr);
    gtk_box_append(GTK_BOX(box), lock);
  }

  auto add_check = [&](const char *label, bool active, bool sensitive, const std::function<void()> &action) {
    GtkWidget *check = gtk_check_button_new_with_label(label);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check), active);
    gtk_widget_set_sensitive(check, sensitive);
    auto *cb = new std::function<void()>(action);
    g_object_set_data_full(G_OBJECT(check), "action", cb, [](gpointer p) { delete static_cast<std::function<void()> *>(p); });
    g_signal_connect(check, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                       if (!gtk_check_button_get_active(button)) {
                         return;
                       }
                       auto *fn = static_cast<std::function<void()> *>(g_object_get_data(G_OBJECT(button), "action"));
                       gtk_popover_popdown(GTK_POPOVER(data));
                       if (fn && *fn) {
                         (*fn)();
                       }
                     }),
                     popover);
    gtk_box_append(GTK_BOX(box), check);
  };

  add_check(Translations::CStr("Sort ascending"),
            PlaylistHeaderSort::AscendingChecked(column, sort_column_, sort_descending_), true, [this, column]() {
              if (sort_ && PlaylistHeaderSort::ShouldApplyExplicit(column, sort_column_, sort_descending_, PlaylistSortOrder::Ascending)) {
                sort_(column, PlaylistSortOrder::Ascending);
              }
            });
  add_check(Translations::CStr("Sort descending"),
            PlaylistHeaderSort::DescendingChecked(column, sort_column_, sort_descending_), true, [this, column]() {
              if (sort_ && PlaylistHeaderSort::ShouldApplyExplicit(column, sort_column_, sort_descending_, PlaylistSortOrder::Descending)) {
                sort_(column, PlaylistSortOrder::Descending);
              }
            });
  GtkWidget *clear = gtk_button_new_with_label(Translations::CStr("Clear sorting"));
  gtk_widget_add_css_class(clear, "flat");
  gtk_widget_set_halign(clear, GTK_ALIGN_FILL);
  gtk_widget_set_sensitive(clear, PlaylistHeaderSort::ClearEnabled(sort_column_));
  auto *clear_cb = new std::function<void()>([this, column]() {
    if (sort_ && PlaylistHeaderSort::ShouldApplyExplicit(column, sort_column_, sort_descending_, PlaylistSortOrder::Clear)) {
      sort_(column, PlaylistSortOrder::Clear);
    }
  });
  g_object_set_data_full(G_OBJECT(clear), "action", clear_cb, [](gpointer p) { delete static_cast<std::function<void()> *>(p); });
  g_signal_connect(clear, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                     auto *fn = static_cast<std::function<void()> *>(g_object_get_data(G_OBJECT(btn), "action"));
                     gtk_popover_popdown(GTK_POPOVER(data));
                     if (fn && *fn) {
                       (*fn)();
                     }
                   }),
                   popover);
  gtk_box_append(GTK_BOX(box), clear);

  const PlaylistColumnAlign align = PlaylistColumnLayout::Alignment(column);
  add_check(Translations::CStr("Align left"), align == PlaylistColumnAlign::Left, true, [this, column]() {
    PlaylistColumnLayout::SetAlignment(column, PlaylistColumnAlign::Left);
    NotifyLayoutChanged();
  });
  add_check(Translations::CStr("Align center"), align == PlaylistColumnAlign::Center, true, [this, column]() {
    PlaylistColumnLayout::SetAlignment(column, PlaylistColumnAlign::Center);
    NotifyLayoutChanged();
  });
  add_check(Translations::CStr("Align right"), align == PlaylistColumnAlign::Right, true, [this, column]() {
    PlaylistColumnLayout::SetAlignment(column, PlaylistColumnAlign::Right);
    NotifyLayoutChanged();
  });
  add_button(Translations::CStr("Move left"), [this, column]() {
    if (PlaylistColumnLayout::Move(column, -1)) {
      NotifyLayoutChanged();
    }
  });
  add_button(Translations::CStr("Move right"), [this, column]() {
    if (PlaylistColumnLayout::Move(column, 1)) {
      NotifyLayoutChanged();
    }
  });

  gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
  for (int i = 0; i < static_cast<int>(PlaylistColumn::Count); ++i) {
    const auto toggle = static_cast<PlaylistColumn>(i);
    const std::string title = PlaylistDelegates::ColumnTitle(toggle);
    if (title.empty() || !PlaylistMoodColumn::ShouldOffer(toggle)) {
      continue;
    }
    GtkWidget *check = gtk_check_button_new_with_label(title.c_str());
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check), PlaylistColumnLayout::IsVisible(toggle));
    g_object_set_data(G_OBJECT(check), "column", GINT_TO_POINTER(i + 1));
    g_signal_connect(check, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                       auto *self = static_cast<PlaylistHeader *>(data);
                       PlaylistColumnLayout::ToggleVisible(
                           static_cast<PlaylistColumn>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "column")) - 1));
                       self->NotifyLayoutChanged();
                     }),
                     this);
    gtk_box_append(GTK_BOX(box), check);
  }

  gtk_popover_set_child(GTK_POPOVER(popover), box);
  gtk_popover_popup(GTK_POPOVER(popover));
}
