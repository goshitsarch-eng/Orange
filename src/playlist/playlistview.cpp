#include "playlist/playlistview.h"

#include "constants/playlistsettings.h"
#include "core/settings.h"
#include "playlist/playlistbehaviour.h"
#include "playlist/playlistcolumnlayout.h"
#include "playlist/playlistratingclick.h"
#include "playlist/playlistfilter.h"
#include "playlist/playlistlook.h"
#include "utilities/strutils.h"
#include "utilities/styleutils.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

PlaylistView::PlaylistView() {
  header_ = std::make_unique<PlaylistHeader>();
  header_->SetSortCallback([this](PlaylistColumn column, PlaylistSortOrder order) {
    if (sort_) {
      sort_(column, order);
    }
  });
  header_->SetLayoutChangedCallback([this]() { Refresh(playlist_); });
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
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(widget_, keys);
  g_signal_connect(keys, "key-pressed", G_CALLBACK(+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     auto *self = static_cast<PlaylistView *>(data);
                     if (keyval == GDK_KEY_F2 && self->edit_request_) {
                       self->edit_request_();
                       return TRUE;
                     }
                     return FALSE;
                   }),
                   this);
  GtkDropTarget *target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
#ifdef GDK_TYPE_FILE_LIST
  GType types[] = {G_TYPE_STRING, GDK_TYPE_FILE_LIST};
  gtk_drop_target_set_gtypes(target, types, 2);
#endif
  gtk_drop_target_set_actions(target, static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE));
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(target));
  g_signal_connect(target, "drop", G_CALLBACK(+[](GtkDropTarget *, const GValue *value, gdouble, gdouble y, gpointer data) -> gboolean {
                     return static_cast<PlaylistView *>(data)->OnDrop(value, y);
                   }),
                   this);
}

void PlaylistView::SetDropUrlsCallback(DropUrlsCallback callback) { drop_urls_ = std::move(callback); }

void PlaylistView::SetReorderCallback(ReorderCallback callback) { reorder_ = std::move(callback); }

void PlaylistView::SetRateCallback(RateCallback callback) { rate_ = std::move(callback); }

int PlaylistView::RowAtY(double y) const {
  if (!grid_) {
    return 0;
  }
  GtkWidget *child = gtk_widget_get_first_child(grid_);
  if (child) {
    child = gtk_widget_get_next_sibling(child);
  }
  int last_index = 0;
  while (child) {
    graphene_rect_t bounds{};
    if (gtk_widget_compute_bounds(child, widget_, &bounds)) {
      const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-index"));
      if (y < bounds.origin.y + bounds.size.height) {
        return index;
      }
      last_index = index + 1;
    }
    child = gtk_widget_get_next_sibling(child);
  }
  return last_index;
}

gboolean PlaylistView::OnDrop(const GValue *value, double y) {
  const int row = RowAtY(y);
  std::vector<std::string> urls;
  if (G_VALUE_HOLDS_STRING(value)) {
    const char *text = g_value_get_string(value);
    if (text && std::string(text).rfind("strawberry-playlist-rows:", 0) == 0) {
      std::vector<int> rows;
      for (const std::string &part : StrUtils::Split(text + std::strlen("strawberry-playlist-rows:"), ',')) {
        if (!part.empty()) {
          rows.push_back(std::atoi(part.c_str()));
        }
      }
      if (reorder_ && !rows.empty()) {
        reorder_(rows, row);
        return TRUE;
      }
    }
    for (const std::string &part : StrUtils::Split(text ? text : "", '\n')) {
      std::string url = part;
      if (!url.empty() && url.back() == '\r') {
        url.pop_back();
      }
      if (!url.empty()) {
        urls.push_back(url);
      }
    }
  }
#ifdef GDK_TYPE_FILE_LIST
  if (G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST)) {
    auto *list = static_cast<GdkFileList *>(g_value_get_boxed(value));
    GSList *files = gdk_file_list_get_files(list);
    for (GSList *item = files; item; item = item->next) {
      gchar *uri = g_file_get_uri(G_FILE(item->data));
      if (uri) {
        urls.emplace_back(uri);
        g_free(uri);
      }
    }
  }
#endif
  if (drop_urls_ && !urls.empty()) {
    drop_urls_(urls, row);
    return TRUE;
  }
  return FALSE;
}

void PlaylistView::SetupRowDrag(GtkWidget *row, int index) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_MOVE);
  g_object_set_data(G_OBJECT(src), "row", GINT_TO_POINTER(index + 1));
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<PlaylistView *>(data);
                     const int r = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(s), "row")) - 1;
                     std::vector<int> rows = self->selected_rows_;
                     if (std::find(rows.begin(), rows.end(), r) == rows.end()) {
                       rows = {r};
                     }
                     if (rows.empty() || r < 0) {
                       return nullptr;
                     }
                     std::string payload = "strawberry-playlist-rows:";
                     for (size_t i = 0; i < rows.size(); ++i) {
                       if (i) {
                         payload += ",";
                       }
                       payload += std::to_string(rows[i]);
                     }
                     GValue v = G_VALUE_INIT;
                     g_value_init(&v, G_TYPE_STRING);
                     g_value_set_string(&v, payload.c_str());
                     GdkContentProvider *provider = gdk_content_provider_new_for_value(&v);
                     g_value_unset(&v);
                     return provider;
                   })),
                   this);
  gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(src));
}

void PlaylistView::SetFilterString(const std::string &filter) { filter_ = filter; }

void PlaylistView::SetSelectedRows(const std::vector<int> &rows) { selected_rows_ = rows; }

void PlaylistView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void PlaylistView::SetSelectCallback(SelectCallback callback) { select_ = std::move(callback); }

void PlaylistView::SetSortCallback(SortCallback callback) { sort_ = std::move(callback); }

void PlaylistView::SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }

void PlaylistView::SetEditRequestCallback(EditRequestCallback callback) { edit_request_ = std::move(callback); }

void PlaylistView::SetEditCommitCallback(EditCommitCallback callback) { edit_commit_ = std::move(callback); }

void PlaylistView::RecordClickedColumn(GtkWidget *row, double x) {
  last_click_cell_x_ = 0;
  last_click_cell_width_ = 0;
  for (GtkWidget *child = gtk_widget_get_first_child(row); child; child = gtk_widget_get_next_sibling(child)) {
    const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "column"));
    if (stored <= 0) {
      continue;
    }
    graphene_rect_t bounds;
    if (!gtk_widget_compute_bounds(child, row, &bounds)) {
      continue;
    }
    if (x >= bounds.origin.x && x < bounds.origin.x + bounds.size.width) {
      last_clicked_column_ = static_cast<PlaylistColumn>(stored - 1);
      last_click_cell_x_ = x - bounds.origin.x;
      last_click_cell_width_ = bounds.size.width;
      return;
    }
  }
}

void PlaylistView::Clear() {
  GtkWidget *child = gtk_widget_get_first_child(grid_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_widget_unparent(child);
    child = next;
  }
}

void PlaylistView::Refresh(Playlist *playlist) {
  playlist_ = playlist;
  Clear();
  header_->Rebuild();
  gtk_box_append(GTK_BOX(grid_), header_->widget());
  if (!playlist) {
    visible_count_ = 0;
    return;
  }
  playlist->SetFilterString(filter_);
  PlaylistFilter filter;
  filter.SetFilterString(filter_);
  const int current = playlist->current_row();
  Settings look;
  look.BeginGroup(PlaylistSettings::kSettingsGroup);
  const bool alternating = look.BoolValue(PlaylistSettings::kAlternatingRowColors, PlaylistSettings::kDefaultAlternatingRowColors);
  const bool glow = look.BoolValue(PlaylistSettings::kGlowEffect, PlaylistSettings::kDefaultGlowEffect);
  const bool bars = look.BoolValue(PlaylistSettings::kShowBars, PlaylistSettings::kDefaultShowBars);
  StyleUtils::LoadCss(PlaylistLook::CombinedCss(alternating, glow, bars, playback_progress_));
  visible_count_ = 0;
  for (int index = 0; index < playlist->row_count(); ++index) {
    const Song &song = playlist->songs()[static_cast<size_t>(index)];
    if (!filter.Accepts(song)) {
      continue;
    }
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(row, "activatable");
    gtk_widget_add_css_class(row, "playlist-row");
    if (alternating && (visible_count_ % 2) == 1) {
      gtk_widget_add_css_class(row, "playlist-alt");
    }
    if (index == current) {
      gtk_widget_add_css_class(row, "accent");
      gtk_widget_add_css_class(row, "playlist-playing");
      if (glow) {
        gtk_widget_add_css_class(row, "playlist-glow");
      }
      if (bars) {
        gtk_widget_add_css_class(row, "playlist-bars");
      }
    }
    if (song.skipped() || PlaylistBehaviour::ShouldGreyout(song)) {
      gtk_widget_add_css_class(row, "dim-label");
    }
    if (PlaylistBehaviour::ShouldGreyout(song)) {
      gtk_widget_add_css_class(row, "playlist-unavailable");
    }
    if (std::find(selected_rows_.begin(), selected_rows_.end(), index) != selected_rows_.end()) {
      gtk_widget_add_css_class(row, "card");
    }
    for (PlaylistColumn column : PlaylistColumnLayout::Visible()) {
      GtkWidget *label = gtk_label_new(PlaylistDelegates::ColumnText(song, column).c_str());
      gtk_label_set_xalign(GTK_LABEL(label), PlaylistColumnLayout::XAlign(column));
      gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
      gtk_widget_set_margin_start(label, 6);
      gtk_widget_set_margin_end(label, 6);
      if (PlaylistColumnLayout::StretchColumn(column)) {
        gtk_widget_set_hexpand(label, TRUE);
      } else {
        gtk_widget_set_size_request(label, PlaylistDelegates::ColumnWidth(column), -1);
      }
      g_object_set_data(G_OBJECT(label), "column", GINT_TO_POINTER(static_cast<int>(column) + 1));
      gtk_box_append(GTK_BOX(row), label);
    }
    g_object_set_data(G_OBJECT(row), "row-index", GINT_TO_POINTER(index));
    GtkGesture *click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(click));
    g_signal_connect(click, "pressed", G_CALLBACK(+[](GtkGestureClick *gesture, gint n_press, gdouble x, gdouble, gpointer data) {
                       auto *self = static_cast<PlaylistView *>(data);
                       GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "row-index"));
                       const GdkModifierType mods = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
                       self->RecordClickedColumn(widget, x);
                       if (self->select_) {
                         self->select_(index, (mods & GDK_CONTROL_MASK) != 0);
                       }
                       bool rated = false;
                       float rating = -1.0f;
                       if (self->rate_ &&
                           PlaylistRatingClick::ShouldRate(self->last_clicked_column_, PlaylistColumnLayout::RatingLocked(),
                                                           static_cast<int>(self->last_click_cell_x_),
                                                           static_cast<int>(self->last_click_cell_width_), &rating)) {
                         self->rate_(index, rating);
                         rated = true;
                       }
                       if (!rated && n_press >= 2 && self->activate_) {
                         self->activate_(index);
                       }
                     }),
                     this);
    SetupRowDrag(row, index);
    gtk_box_append(GTK_BOX(grid_), row);
    ++visible_count_;
  }
}

void PlaylistView::StartInlineEdit(int row, PlaylistColumn column) {
  if (!PlaylistDelegates::ColumnIsEditable(column)) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(grid_);
  while (child) {
    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-index")) == row) {
      for (GtkWidget *cell = gtk_widget_get_first_child(child); cell; cell = gtk_widget_get_next_sibling(cell)) {
        const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));
        if (stored - 1 != static_cast<int>(column)) {
          continue;
        }
        const char *current = GTK_IS_LABEL(cell) ? gtk_label_get_text(GTK_LABEL(cell)) : "";
        GtkWidget *entry = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(entry), current);
        gtk_widget_set_hexpand(entry, PlaylistColumnLayout::StretchColumn(column));
        gtk_widget_set_size_request(entry, PlaylistDelegates::ColumnWidth(column), -1);
        g_object_set_data(G_OBJECT(entry), "column", GINT_TO_POINTER(stored));
        g_object_set_data(G_OBJECT(entry), "row-index", GINT_TO_POINTER(row));
        GtkWidget *prev = gtk_widget_get_prev_sibling(cell);
        gtk_widget_unparent(cell);
        if (prev) {
          gtk_box_insert_child_after(GTK_BOX(child), entry, prev);
        } else {
          gtk_box_prepend(GTK_BOX(child), entry);
        }
        g_signal_connect(entry, "activate", G_CALLBACK(+[](GtkEntry *widget, gpointer data) {
                           auto *self = static_cast<PlaylistView *>(data);
                           const int edited_row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "row-index"));
                           const PlaylistColumn edited_column =
                               static_cast<PlaylistColumn>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "column")) - 1);
                           if (self->edit_commit_) {
                             self->edit_commit_(edited_row, edited_column, gtk_editable_get_text(GTK_EDITABLE(widget)));
                           }
                         }),
                         this);
        gtk_widget_grab_focus(entry);
        return;
      }
    }
    child = gtk_widget_get_next_sibling(child);
  }
}

void PlaylistView::SetPlaybackProgress(double progress) {
  playback_progress_ = std::clamp(progress, 0.0, 1.0);
  Settings look;
  look.BeginGroup(PlaylistSettings::kSettingsGroup);
  StyleUtils::LoadCss(PlaylistLook::CombinedCss(look.BoolValue(PlaylistSettings::kAlternatingRowColors, PlaylistSettings::kDefaultAlternatingRowColors),
                                                look.BoolValue(PlaylistSettings::kGlowEffect, PlaylistSettings::kDefaultGlowEffect),
                                                look.BoolValue(PlaylistSettings::kShowBars, PlaylistSettings::kDefaultShowBars), playback_progress_));
}

void PlaylistView::ScrollToRow(int row) {
  GtkWidget *child = gtk_widget_get_first_child(grid_);
  while (child) {
    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-index")) == row) {
      graphene_rect_t bounds;
      if (gtk_widget_compute_bounds(child, widget_, &bounds)) {
        GtkAdjustment *adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(widget_));
        gtk_adjustment_set_value(adjust, bounds.origin.y);
      }
      return;
    }
    child = gtk_widget_get_next_sibling(child);
  }
}
