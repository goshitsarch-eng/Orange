#include "playlist/playlistview.h"

#include "playlist/playlistfilter.h"
#include "playlist/playlistheader.h"

#include <algorithm>

PlaylistView::PlaylistView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_hexpand(widget_, TRUE);
  gtk_widget_set_vexpand(widget_, TRUE);
  grid_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), grid_);
  GtkGesture *gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(grid_, GTK_EVENT_CONTROLLER(gesture));
  g_signal_connect(gesture, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble x, gdouble y, gpointer data) {
                     auto *self = static_cast<PlaylistView *>(data);
                     if (self->menu_) {
                       self->menu_(x, y);
                     }
                   }),
                   this);
}

void PlaylistView::SetFilterString(const std::string &filter) { filter_ = filter; }

void PlaylistView::SetSelectedRows(const std::vector<int> &rows) { selected_rows_ = rows; }

void PlaylistView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void PlaylistView::SetSelectCallback(SelectCallback callback) { select_ = std::move(callback); }

void PlaylistView::SetSortCallback(SortCallback callback) { sort_ = std::move(callback); }

void PlaylistView::SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }

void PlaylistView::Clear() {
  GtkWidget *child = gtk_widget_get_first_child(grid_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_widget_unparent(child);
    child = next;
  }
}

void PlaylistView::Refresh(Playlist *playlist) {
  Clear();
  PlaylistHeader header;
  header.Rebuild([this](PlaylistColumn column) {
    if (sort_) {
      sort_(column);
    }
  });
  gtk_box_append(GTK_BOX(grid_), header.widget());
  if (!playlist) {
    visible_count_ = 0;
    return;
  }
  PlaylistFilter filter;
  filter.SetFilterString(filter_);
  const int current = playlist->current_row();
  visible_count_ = 0;
  for (int index = 0; index < playlist->row_count(); ++index) {
    const Song &song = playlist->songs()[static_cast<size_t>(index)];
    if (!filter.Accepts(song)) {
      continue;
    }
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(row, "activatable");
    if (index == current) {
      gtk_widget_add_css_class(row, "accent");
    }
    if (std::find(selected_rows_.begin(), selected_rows_.end(), index) != selected_rows_.end()) {
      gtk_widget_add_css_class(row, "card");
    }
    for (int i = 0; i < static_cast<int>(PlaylistColumn::Count); ++i) {
      const auto column = static_cast<PlaylistColumn>(i);
      if (!PlaylistDelegates::ColumnVisible(column)) {
        continue;
      }
      GtkWidget *label = gtk_label_new(PlaylistDelegates::ColumnText(song, column).c_str());
      gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
      gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
      gtk_widget_set_margin_start(label, 6);
      gtk_widget_set_margin_end(label, 6);
      if (column == PlaylistColumn::Title) {
        gtk_widget_set_hexpand(label, TRUE);
      } else {
        gtk_widget_set_size_request(label, PlaylistDelegates::ColumnWidth(column), -1);
      }
      gtk_box_append(GTK_BOX(row), label);
    }
    g_object_set_data(G_OBJECT(row), "row-index", GINT_TO_POINTER(index));
    GtkGesture *click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(click));
    g_signal_connect(click, "pressed", G_CALLBACK(+[](GtkGestureClick *gesture, gint n_press, gdouble, gdouble, gpointer data) {
                       auto *self = static_cast<PlaylistView *>(data);
                       GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "row-index"));
                       const GdkModifierType mods = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
                       if (self->select_) {
                         self->select_(index, (mods & GDK_CONTROL_MASK) != 0);
                       }
                       if (n_press >= 2 && self->activate_) {
                         self->activate_(index);
                       }
                     }),
                     this);
    gtk_box_append(GTK_BOX(grid_), row);
    ++visible_count_;
  }
}
