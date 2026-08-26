#include "playlist/playlistheader.h"

#include "playlist/playlistcolumnlayout.h"
#include "translations/translations.h"

#include <string>

PlaylistHeader::PlaylistHeader() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(widget_, "toolbar");
  gtk_widget_add_css_class(widget_, "strawberry-playlist-buttons");
  GtkGesture *gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(gesture));
  g_signal_connect(gesture, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble x, gdouble, gpointer data) {
                     auto *self = static_cast<PlaylistHeader *>(data);
                     self->ShowMenu(self->ColumnAtX(x));
                   }),
                   this);
}

void PlaylistHeader::Rebuild() {
  GtkWidget *child = gtk_widget_get_first_child(widget_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_widget_unparent(child);
    child = next;
  }
  for (PlaylistColumn column : PlaylistColumnLayout::Visible()) {
    GtkWidget *button = gtk_button_new_with_label(PlaylistDelegates::ColumnTitle(column).c_str());
    gtk_widget_add_css_class(button, "flat");
    if (PlaylistColumnLayout::StretchColumn(column)) {
      gtk_widget_set_hexpand(button, TRUE);
    } else {
      gtk_widget_set_size_request(button, PlaylistDelegates::ColumnWidth(column), -1);
    }
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

  const std::string hide_label = std::string(Translations::CStr("Hide")) + " " + PlaylistDelegates::ColumnTitle(column);
  add_button(hide_label.c_str(), [this, column]() {
    PlaylistColumnLayout::Hide(column);
    NotifyLayoutChanged();
  });

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

  add_button(Translations::CStr("Sort ascending"), [this, column]() {
    if (sort_) {
      sort_(column, PlaylistSortOrder::Ascending);
    }
  });
  add_button(Translations::CStr("Sort descending"), [this, column]() {
    if (sort_) {
      sort_(column, PlaylistSortOrder::Descending);
    }
  });
  add_button(Translations::CStr("Clear sorting"), [this, column]() {
    if (sort_) {
      sort_(column, PlaylistSortOrder::Clear);
    }
  });
  add_button(Translations::CStr("Align left"), [this, column]() {
    PlaylistColumnLayout::SetAlignment(column, PlaylistColumnAlign::Left);
    NotifyLayoutChanged();
  });
  add_button(Translations::CStr("Align center"), [this, column]() {
    PlaylistColumnLayout::SetAlignment(column, PlaylistColumnAlign::Center);
    NotifyLayoutChanged();
  });
  add_button(Translations::CStr("Align right"), [this, column]() {
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
    if (title.empty()) {
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
