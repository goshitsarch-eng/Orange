#include "playlist/playlistview.h"

#include "core/appearance.h"
#include "core/appearancebackgroundfade.h"
#include "constants/playlistsettings.h"
#include "moodbar/moodbarcell.h"
#include "core/settings.h"
#include "playlist/playlistdragpayload.h"
#include "playlist/playlistautoscroll.h"
#include "playlist/playlistbehaviour.h"
#include "playlist/playlistclipboard.h"
#include "playlist/playlistcolumnlayout.h"
#include "playlist/playlisteditcolumn.h"
#include "playlist/playlisteditorder.h"
#include "playlist/playlisteditpolicy.h"
#include "playlist/playlistplayingicon.h"
#include "playlist/playlistrating.h"
#include "playlist/playlistratingclick.h"
#include "playlist/playlistratinghover.h"
#include "playlist/playlistdropindicator.h"
#include "playlist/playlistfilter.h"
#include "playlist/playlistfilterdelay.h"
#include "playlist/playlistfilterfocus.h"
#include "playlist/playlistrestorescroll.h"
#include "playlist/playlistfilterempty.h"
#include "playlist/playlistkeyboard.h"
#include "playlist/playlistmenu.h"
#include "playlist/playlistlook.h"
#include "playlist/playliststopafter.h"
#include "playlist/playlisttagcompletion.h"
#include "translations/translations.h"
#include "utilities/strutils.h"
#include "utilities/styleutils.h"
#include "widgets/listboxkeyboard.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

PlaylistView::PlaylistView() {
  moodbar_ = std::make_unique<MoodbarItemDelegate>();
  moodbar_->SetUpdatedCallback([this]() { QueueDrawMoodbars(); });
  header_ = std::make_unique<PlaylistHeader>();
  header_->SetSortCallback([this](PlaylistColumn column, PlaylistSortOrder order) {
    if (sort_) {
      sort_(column, order);
    }
  });
  header_->SetLayoutChangedCallback([this]() { Refresh(playlist_); });
  header_->SetWidthsChangedCallback([this]() { ApplyColumnWidths(); });
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_hexpand(widget_, TRUE);
  gtk_widget_set_vexpand(widget_, TRUE);
  current_bg_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(current_bg_, "strawberry-playlist-viewport");
  gtk_widget_set_hexpand(current_bg_, TRUE);
  gtk_widget_set_vexpand(current_bg_, TRUE);
  previous_bg_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(previous_bg_, "strawberry-playlist-previous-background");
  gtk_widget_set_hexpand(previous_bg_, TRUE);
  gtk_widget_set_vexpand(previous_bg_, TRUE);
  gtk_widget_set_can_target(previous_bg_, FALSE);
  gtk_widget_set_opacity(previous_bg_, 0.0);
  bg_overlay_ = gtk_overlay_new();
  gtk_overlay_set_child(GTK_OVERLAY(bg_overlay_), current_bg_);
  gtk_overlay_add_overlay(GTK_OVERLAY(bg_overlay_), previous_bg_);
  grid_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  overlay_ = gtk_overlay_new();
  gtk_overlay_set_child(GTK_OVERLAY(overlay_), grid_);
  gtk_widget_set_hexpand(overlay_, TRUE);
  gtk_widget_set_vexpand(overlay_, TRUE);
  drop_overlay_ = gtk_drawing_area_new();
  gtk_widget_set_can_target(drop_overlay_, FALSE);
  gtk_widget_set_hexpand(drop_overlay_, TRUE);
  gtk_widget_set_vexpand(drop_overlay_, TRUE);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drop_overlay_),
                                 +[](GtkDrawingArea *, cairo_t *cr, int width, int, gpointer data) {
                                   auto *self = static_cast<PlaylistView *>(data);
                                   if (!PlaylistDropIndicator::Active(self->drop_state_)) {
                                     return;
                                   }
                                   const double y = self->drop_state_.line_y;
                                   cairo_pattern_t *grad = cairo_pattern_create_linear(0, y - PlaylistDropIndicator::kGradientWidth, 0,
                                                                                       y + PlaylistDropIndicator::kGradientWidth);
                                   cairo_pattern_add_color_stop_rgba(grad, 0.0, 0.2, 0.5, 1.0, 0.0);
                                   cairo_pattern_add_color_stop_rgba(grad, 0.5, 0.2, 0.5, 1.0, 0.35);
                                   cairo_pattern_add_color_stop_rgba(grad, 1.0, 0.2, 0.5, 1.0, 0.0);
                                   cairo_set_source(cr, grad);
                                   cairo_rectangle(cr, 0, y - PlaylistDropIndicator::kGradientWidth, width,
                                                   PlaylistDropIndicator::kGradientWidth * 2);
                                   cairo_fill(cr);
                                   cairo_pattern_destroy(grad);
                                   cairo_set_source_rgb(cr, 0.2, 0.5, 1.0);
                                   cairo_rectangle(cr, 0, y, width, PlaylistDropIndicator::kLineWidth);
                                   cairo_fill(cr);
                                 },
                                 this, nullptr);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay_), drop_overlay_);
  no_matches_ = gtk_label_new(Translations::CStr(PlaylistFilterEmpty::Message()));
  gtk_label_set_wrap(GTK_LABEL(no_matches_), TRUE);
  gtk_label_set_justify(GTK_LABEL(no_matches_), GTK_JUSTIFY_CENTER);
  gtk_label_set_xalign(GTK_LABEL(no_matches_), 0.5f);
  gtk_widget_add_css_class(no_matches_, "dim-label");
  gtk_widget_set_halign(no_matches_, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(no_matches_, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(no_matches_, 24);
  gtk_widget_set_margin_end(no_matches_, 24);
  gtk_widget_set_can_target(no_matches_, FALSE);
  gtk_widget_set_visible(no_matches_, FALSE);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay_), no_matches_);
  root_overlay_ = gtk_overlay_new();
  gtk_overlay_set_child(GTK_OVERLAY(root_overlay_), bg_overlay_);
  gtk_overlay_add_overlay(GTK_OVERLAY(root_overlay_), overlay_);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), root_overlay_);
  GtkGesture *gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(grid_, GTK_EVENT_CONTROLLER(gesture));
  g_signal_connect(gesture, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble x, gdouble y, gpointer data) {
                     auto *self = static_cast<PlaylistView *>(data);
                     self->RememberClickAt(x, y);
                     if (self->menu_) {
                       self->menu_(x, y);
                     }
                   }),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(widget_, keys);
  gtk_widget_set_focusable(widget_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     return static_cast<PlaylistView *>(data)->OnKeyPressed(keyval, state);
                   })),
                   this);
  GtkDropTarget *target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
#ifdef GDK_TYPE_FILE_LIST
  GType types[] = {G_TYPE_STRING, GDK_TYPE_FILE_LIST};
  gtk_drop_target_set_gtypes(target, types, 2);
#endif
  gtk_drop_target_set_actions(target, static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE));
  gtk_drop_target_set_preload(target, TRUE);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(target));
  g_signal_connect(target, "motion", G_CALLBACK(+[](GtkDropTarget *, gdouble, gdouble y, gpointer data) -> GdkDragAction {
                     auto *self = static_cast<PlaylistView *>(data);
                     self->UpdateDropIndicator(y);
                     return GDK_ACTION_COPY;
                   }),
                   this);
  g_signal_connect(target, "leave", G_CALLBACK(+[](GtkDropTarget *, gpointer data) {
                     static_cast<PlaylistView *>(data)->ClearDropIndicator();
                   }),
                   this);
  g_signal_connect(target, "drop", G_CALLBACK(+[](GtkDropTarget *, const GValue *value, gdouble, gdouble y, gpointer data) -> gboolean {
                     return static_cast<PlaylistView *>(data)->OnDrop(value, y);
                   }),
                   this);
  g_signal_connect(widget_, "notify::width", G_CALLBACK(+[](GObject *, GParamSpec *, gpointer data) {
                     static_cast<PlaylistView *>(data)->ApplyColumnWidths();
                   }),
                   this);
  g_signal_connect(widget_, "map", G_CALLBACK(+[](GtkWidget *, gpointer data) { static_cast<PlaylistView *>(data)->OnShown(); }), this);
  g_signal_connect(widget_, "unmap", G_CALLBACK(+[](GtkWidget *, gpointer data) { static_cast<PlaylistView *>(data)->OnHidden(); }), this);
}

void PlaylistView::SetDropUrlsCallback(DropUrlsCallback callback) { drop_urls_ = std::move(callback); }

void PlaylistView::SetReorderCallback(ReorderCallback callback) { reorder_ = std::move(callback); }

void PlaylistView::SetCrossDropCallback(CrossDropCallback callback) { cross_drop_ = std::move(callback); }

void PlaylistView::SetRateCallback(RateCallback callback) { rate_ = std::move(callback); }

void PlaylistView::SetQueuePositionCallback(QueuePositionCallback callback) { queue_position_ = std::move(callback); }

void PlaylistView::SetDeleteCallback(DeleteCallback callback) { delete_ = std::move(callback); }

PlaylistView::~PlaylistView() {
  StopGlowTimer();
  StopBackgroundFade();
  if (inhibit_timeout_) {
    g_source_remove(inhibit_timeout_);
  }
  ResetTypeAhead();
}

void PlaylistView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

void PlaylistView::SetFocusFilterCallback(FocusFilterCallback callback) { focus_filter_ = std::move(callback); }

void PlaylistView::SetPlayPauseCallback(PlayPauseCallback callback) { play_pause_ = std::move(callback); }

void PlaylistView::SetSeekBackwardCallback(SeekCallback callback) { seek_backward_ = std::move(callback); }

void PlaylistView::SetSeekForwardCallback(SeekCallback callback) { seek_forward_ = std::move(callback); }

void PlaylistView::FilterReturnPressed() {
  if (!activate_ || visible_rows_.empty()) {
    return;
  }
  const int row = selected_rows_.empty() ? visible_rows_.front() : selected_rows_.front();
  activate_(row);
}

void PlaylistView::FocusAndMove(unsigned keyval) {
  gtk_widget_grab_focus(widget_);
  OnKeyPressed(keyval, static_cast<GdkModifierType>(0));
}

gboolean PlaylistView::OnKeyPressed(guint keyval, GdkModifierType state) {
  if (PlaylistMenu::IsKeyboardTrigger(keyval, static_cast<unsigned>(state)) && menu_) {
    menu_(0, PlaylistMenu::kKeyboardY);
    return TRUE;
  }
  const PlaylistKeyboard::Action key_action = PlaylistKeyboard::FromKey(keyval, state, GDK_CONTROL_MASK);
  if (key_action == PlaylistKeyboard::Action::PlayPause && play_pause_) {
    play_pause_();
    return TRUE;
  }
  if (key_action == PlaylistKeyboard::Action::SeekBack && seek_backward_) {
    seek_backward_();
    return TRUE;
  }
  if (key_action == PlaylistKeyboard::Action::SeekForward && seek_forward_) {
    seek_forward_();
    return TRUE;
  }
  if (PlaylistClipboard::IsCopyShortcut(keyval, state, GDK_CONTROL_MASK)) {
    CopyCurrentToClipboard();
    return TRUE;
  }
  const gunichar ch = gdk_keyval_to_unicode(keyval);
  const bool printable_nonspace = ch && g_unichar_isprint(ch) && ch != ' ';
  if (PlaylistFilterFocus::ShouldRouteToFilter(keyval, state, printable_nonspace, GDK_CONTROL_MASK)) {
    ResetTypeAhead();
    gchar utf8[8] = {};
    if (ch) {
      g_unichar_to_utf8(ch, utf8);
    }
    if (focus_filter_) {
      focus_filter_(keyval, utf8);
    }
    return TRUE;
  }
  if (keyval == GDK_KEY_F2 && edit_request_) {
    edit_request_();
    return TRUE;
  }
  const ListBoxKeyboard::Action action = ListBoxKeyboard::FromKey(keyval);
  if (action == ListBoxKeyboard::Action::Activate && activate_ && !selected_rows_.empty()) {
    activate_(selected_rows_.front());
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::Delete && delete_) {
    delete_();
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::MoveUp || action == ListBoxKeyboard::Action::MoveDown || action == ListBoxKeyboard::Action::Home ||
      action == ListBoxKeyboard::Action::End || action == ListBoxKeyboard::Action::PageUp || action == ListBoxKeyboard::Action::PageDown) {
    int current = -1;
    if (!selected_rows_.empty() && !visible_rows_.empty()) {
      for (size_t i = 0; i < visible_rows_.size(); ++i) {
        if (visible_rows_[i] == selected_rows_.front()) {
          current = static_cast<int>(i);
          break;
        }
      }
    }
    const int next = ListBoxKeyboard::NextIndex(current, static_cast<int>(visible_rows_.size()), action,
                                                ListBoxKeyboard::PageSize(gtk_widget_get_height(widget_)));
    if (next >= 0 && select_) {
      const int next_row = visible_rows_[static_cast<size_t>(next)];
      if (PlaylistEditColumn::ShouldClearLastClicked(last_clicked_row_, next_row)) {
        last_clicked_column_ = PlaylistColumn::Count;
        last_clicked_row_ = -1;
      }
      InhibitAutoscroll();
      select_(next_row, false);
      ScrollToRow(next_row);
    }
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::Escape) {
    ResetTypeAhead();
    return TRUE;
  }
  if (ch && g_unichar_isprint(ch) && PlaylistFilterFocus::ShouldTypeahead(false)) {
    gchar utf8[8] = {};
    typeahead_.append(utf8, static_cast<size_t>(g_unichar_to_utf8(ch, utf8)));
    if (typeahead_timeout_) {
      g_source_remove(typeahead_timeout_);
    }
    typeahead_timeout_ = g_timeout_add(1000, [](gpointer data) -> gboolean {
      auto *self = static_cast<PlaylistView *>(data);
      self->typeahead_timeout_ = 0;
      self->typeahead_.clear();
      return G_SOURCE_REMOVE;
    }, this);
    const int index = ListBoxKeyboard::FirstPrefixIndex(visible_titles_, typeahead_);
    if (index >= 0 && select_) {
      select_(visible_rows_[static_cast<size_t>(index)], false);
      ScrollToRow(visible_rows_[static_cast<size_t>(index)]);
    }
    return TRUE;
  }
  return FALSE;
}

int PlaylistView::RowAtY(double y) const { return RowAtY(y, widget_); }

int PlaylistView::RowAtY(double y, GtkWidget *relative) const {
  if (!grid_ || !relative) {
    return 0;
  }
  GtkWidget *child = gtk_widget_get_first_child(grid_);
  if (child) {
    child = gtk_widget_get_next_sibling(child);
  }
  int last_index = 0;
  while (child) {
    graphene_rect_t bounds{};
    if (gtk_widget_compute_bounds(child, relative, &bounds)) {
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

void PlaylistView::RememberClickAt(double x, double y) {
  if (!grid_) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(grid_);
  if (child) {
    child = gtk_widget_get_next_sibling(child);
  }
  while (child) {
    graphene_rect_t bounds{};
    if (gtk_widget_compute_bounds(child, grid_, &bounds) && y >= bounds.origin.y &&
        y < bounds.origin.y + bounds.size.height) {
      last_clicked_row_ = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-index"));
      RecordClickedColumn(child, x - bounds.origin.x);
      return;
    }
    child = gtk_widget_get_next_sibling(child);
  }
}

void PlaylistView::ClearDropIndicator() {
  drop_state_ = {};
  if (drop_overlay_) {
    gtk_widget_queue_draw(drop_overlay_);
  }
}

void PlaylistView::UpdateDropIndicator(double y) {
  GtkWidget *child = gtk_widget_get_first_child(grid_);
  if (child) {
    child = gtk_widget_get_next_sibling(child);
  }
  double last_bottom = 0;
  int found = -1;
  double row_y = 0;
  double row_h = 0;
  bool has_rows = false;
  while (child) {
    graphene_rect_t bounds{};
    if (gtk_widget_compute_bounds(child, widget_, &bounds)) {
      has_rows = true;
      last_bottom = bounds.origin.y + bounds.size.height;
      const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-index"));
      if (y < last_bottom && found < 0) {
        found = index;
        row_y = bounds.origin.y;
        row_h = bounds.size.height;
      }
    }
    child = gtk_widget_get_next_sibling(child);
  }
  drop_state_ = PlaylistDropIndicator::FromPointer(y, found, row_y, row_h, has_rows, last_bottom);
  if (drop_overlay_) {
    gtk_widget_queue_draw(drop_overlay_);
  }
}

gboolean PlaylistView::OnDrop(const GValue *value, double y) {
  UpdateDropIndicator(y);
  const int row = PlaylistDropIndicator::InsertRow(drop_state_, RowAtY(y));
  ClearDropIndicator();
  std::vector<std::string> urls;
  if (G_VALUE_HOLDS_STRING(value)) {
    const char *text = g_value_get_string(value);
    if (text && PlaylistDragPayload::IsPlaylistRows(text)) {
      const PlaylistDragPayload::Payload payload = PlaylistDragPayload::Decode(text);
      const int dest_id = playlist_ ? playlist_->id() : -1;
      if (cross_drop_ && PlaylistDragPayload::IsCrossPlaylist(payload.source_id, dest_id)) {
        cross_drop_(payload.source_id, payload.rows, row);
        return TRUE;
      }
      if (reorder_ && !payload.rows.empty()) {
        reorder_(payload.rows, row);
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
                     const std::string payload =
                         PlaylistDragPayload::Encode(self->playlist_ ? self->playlist_->id() : -1, rows);
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

void PlaylistView::HandleRatingHover(GtkWidget *row, double x) {
  if (!row) {
    return;
  }
  RecordClickedColumn(row, x);
  const bool locked = PlaylistColumnLayout::RatingLocked();
  if (!PlaylistRatingHover::ShouldHover(last_clicked_column_, locked, false)) {
    ClearRatingHover();
    return;
  }
  const float rating = PlaylistRatingHover::PreviewRating(static_cast<int>(last_click_cell_x_), static_cast<int>(last_click_cell_width_));
  if (!PlaylistRatingHover::IsActive(rating)) {
    ClearRatingHover();
    return;
  }
  hover_rating_row_ = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-index"));
  hover_rating_ = rating;
  gtk_widget_set_cursor_from_name(widget_, PlaylistRatingHover::CursorName());
  UpdateRatingHoverLabels();
}

void PlaylistView::ClearRatingHover() {
  if (hover_rating_row_ < 0 && hover_rating_ < 0.0f) {
    return;
  }
  hover_rating_row_ = -1;
  hover_rating_ = -1.0f;
  if (widget_) {
    gtk_widget_set_cursor(widget_, nullptr);
  }
  UpdateRatingHoverLabels();
}

void PlaylistView::UpdateRatingHoverLabels() {
  if (!playlist_ || !grid_) {
    return;
  }
  for (GtkWidget *row = gtk_widget_get_first_child(grid_); row; row = gtk_widget_get_next_sibling(row)) {
    const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-index"));
    if (index < 0 || index >= playlist_->row_count()) {
      continue;
    }
    const bool preview = PlaylistRatingHover::ShouldPreviewRow(index, hover_rating_row_, selected_rows_);
    const std::string text = PlaylistRatingHover::DisplayText(playlist_->song(index).rating(), hover_rating_, preview);
    for (GtkWidget *cell = gtk_widget_get_first_child(row); cell; cell = gtk_widget_get_next_sibling(cell)) {
      const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));
      if (stored - 1 != static_cast<int>(PlaylistColumn::Rating) || !GTK_IS_LABEL(cell)) {
        continue;
      }
      gtk_label_set_text(GTK_LABEL(cell), text.c_str());
    }
  }
}

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

void PlaylistView::QueueDrawMoodbars() {
  if (!grid_) {
    return;
  }
  GtkWidget *row = gtk_widget_get_first_child(grid_);
  while (row) {
    GtkWidget *cell = gtk_widget_get_first_child(row);
    while (cell) {
      if (g_object_get_data(G_OBJECT(cell), "mood-url")) {
        gtk_widget_queue_draw(cell);
      }
      cell = gtk_widget_get_next_sibling(cell);
    }
    row = gtk_widget_get_next_sibling(row);
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

void PlaylistView::ApplyColumnWidths() {
  const int total = gtk_widget_get_width(widget_);
  header_->SetViewportWidth(total);
  header_->ApplyWidths();
  if (!grid_) {
    return;
  }
  for (GtkWidget *row = gtk_widget_get_first_child(grid_); row; row = gtk_widget_get_next_sibling(row)) {
    if (row == header_->widget()) {
      continue;
    }
    for (GtkWidget *cell = gtk_widget_get_first_child(row); cell; cell = gtk_widget_get_next_sibling(cell)) {
      const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));
      if (stored <= 0) {
        continue;
      }
      gtk_widget_set_hexpand(cell, FALSE);
      gtk_widget_set_size_request(cell, PlaylistColumnLayout::PixelWidth(static_cast<PlaylistColumn>(stored - 1), total), -1);
    }
  }
}

void PlaylistView::Refresh(Playlist *playlist) {
  hover_rating_row_ = -1;
  hover_rating_ = -1.0f;
  playlist_ = playlist;
  Clear();
  header_->SetViewportWidth(gtk_widget_get_width(widget_));
  header_->SetSortState(playlist ? playlist->sort_column() : PlaylistColumn::Count, playlist && playlist->sort_descending());
  header_->Rebuild();
  gtk_box_append(GTK_BOX(grid_), header_->widget());
  if (!playlist) {
    visible_count_ = 0;
    UpdateNoMatchesOverlay();
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
  StyleUtils::LoadCss(PlaylistLook::CombinedCss(alternating, glow, bars, playback_progress_, glow_step_));
  visible_count_ = 0;
  visible_titles_.clear();
  visible_rows_.clear();
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
    if (PlaylistStopAfter::IsRow(playlist->stop_after_row(), index)) {
      gtk_widget_add_css_class(row, PlaylistStopAfter::CssClass());
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
      if (PlaylistPlayingIcon::ShowOnCurrentRow(true)) {
        GtkWidget *icon = gtk_image_new_from_icon_name(PlaylistPlayingIcon::Name(paused_));
        gtk_image_set_pixel_size(GTK_IMAGE(icon), PlaylistPlayingIcon::kPixelSize);
        gtk_widget_set_margin_start(icon, 6);
        gtk_widget_set_valign(icon, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(row), icon);
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
      GtkWidget *cell = nullptr;
      if (column == PlaylistColumn::Moodbar && moodbar_) {
        moodbar_->Ensure(song);
        GtkWidget *area = gtk_drawing_area_new();
        gtk_widget_set_size_request(area, PlaylistColumnLayout::PixelWidth(column, gtk_widget_get_width(widget_)), MoodbarCell::ColumnHeight());
        gtk_widget_set_hexpand(area, FALSE);
        gtk_widget_set_margin_start(area, 6);
        gtk_widget_set_margin_end(area, 6);
        g_object_set_data_full(G_OBJECT(area), "mood-url", g_strdup(song.url().c_str()), g_free);
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area),
                                       +[](GtkDrawingArea *widget, cairo_t *cr, int width, int height, gpointer data) {
                                         auto *self = static_cast<PlaylistView *>(data);
                                         const char *url = static_cast<const char *>(g_object_get_data(G_OBJECT(widget), "mood-url"));
                                         if (!self->moodbar_ || !url) {
                                           return;
                                         }
                                         if (const std::vector<uint8_t> *bytes = self->moodbar_->Peek(url)) {
                                           MoodbarItemDelegate::Paint(cr, width, height, *bytes);
                                         }
                                       },
                                       this, nullptr);
        cell = area;
      } else {
        std::string text = PlaylistDelegates::ColumnText(song, column);
        if (column == PlaylistColumn::Title) {
          text = PlaylistStopAfter::TitleText(text, PlaylistStopAfter::IsRow(playlist->stop_after_row(), index));
        }
        if (column == PlaylistColumn::Queue && queue_position_) {
          const int position = queue_position_(index);
          text = position > 0 ? std::to_string(position) : "";
        }
        GtkWidget *label = gtk_label_new(text.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), PlaylistColumnLayout::XAlign(column));
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
        gtk_widget_set_margin_start(label, 6);
        gtk_widget_set_margin_end(label, 6);
        gtk_widget_set_hexpand(label, FALSE);
        gtk_widget_set_size_request(label, PlaylistColumnLayout::PixelWidth(column, gtk_widget_get_width(widget_)), -1);
        cell = label;
      }
      g_object_set_data(G_OBJECT(cell), "column", GINT_TO_POINTER(static_cast<int>(column) + 1));
      gtk_box_append(GTK_BOX(row), cell);
    }
    g_object_set_data(G_OBJECT(row), "row-index", GINT_TO_POINTER(index));
    visible_titles_.push_back(song.PrettyTitle());
    visible_rows_.push_back(index);
    GtkGesture *click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(click));
    g_signal_connect(click, "pressed", G_CALLBACK(+[](GtkGestureClick *gesture, gint n_press, gdouble x, gdouble, gpointer data) {
                       auto *self = static_cast<PlaylistView *>(data);
                       GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "row-index"));
                       const GdkModifierType mods = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
                       self->last_clicked_row_ = index;
                       self->RecordClickedColumn(widget, x);
                       self->InhibitAutoscroll();
                       const std::vector<int> selected_before = self->selected_rows_;
                       const bool already_selected =
                           std::find(selected_before.begin(), selected_before.end(), index) != selected_before.end();
                       if (self->select_) {
                         self->select_(index, (mods & GDK_CONTROL_MASK) != 0);
                       }
                       bool rated = false;
                       float rating = -1.0f;
                       if (self->rate_ &&
                           PlaylistRatingClick::ShouldRate(self->last_clicked_column_, PlaylistColumnLayout::RatingLocked(),
                                                           static_cast<int>(self->last_click_cell_x_),
                                                           static_cast<int>(self->last_click_cell_width_), &rating)) {
                         self->rate_(PlaylistRating::RowsForStarClick(index, selected_before), rating);
                         rated = true;
                       }
                       Settings settings;
                       settings.BeginGroup(PlaylistSettings::kSettingsGroup);
                       const bool inline_edit =
                           settings.BoolValue(PlaylistSettings::kEditMetadataInline, PlaylistSettings::kDefaultEditMetadataInline);
                       if (!rated && n_press == 1 && self->edit_request_ &&
                           PlaylistEditPolicy::SelectedClickStartsEdit(inline_edit, already_selected)) {
                         self->edit_request_();
                       }
                       if (!rated && n_press >= 2 && self->activate_) {
                         self->activate_(index);
                       }
                     }),
                     this);
    GtkEventController *motion = gtk_event_controller_motion_new();
    gtk_widget_add_controller(row, motion);
    g_signal_connect(motion, "motion", G_CALLBACK(+[](GtkEventControllerMotion *controller, gdouble x, gdouble, gpointer data) {
                       auto *self = static_cast<PlaylistView *>(data);
                       self->HandleRatingHover(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller)), x);
                     }),
                     this);
    g_signal_connect(motion, "leave", G_CALLBACK(+[](GtkEventControllerMotion *, gpointer data) {
                       static_cast<PlaylistView *>(data)->ClearRatingHover();
                     }),
                     this);
    SetupRowDrag(row, index);
    gtk_box_append(GTK_BOX(grid_), row);
    ++visible_count_;
  }
  ApplyColumnWidths();
  UpdateNoMatchesOverlay();
  const int insert_row = playlist->TakeInsertScrollRow();
  if (insert_row >= 0) {
    ScrollToRow(insert_row, false);
  }
}

void PlaylistView::UpdateNoMatchesOverlay() {
  if (!no_matches_) {
    return;
  }
  const int total = playlist_ ? playlist_->row_count() : 0;
  gtk_widget_set_visible(no_matches_, PlaylistFilterEmpty::ShouldShow(total, visible_count_) ? TRUE : FALSE);
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
        gtk_widget_set_hexpand(entry, FALSE);
        gtk_widget_set_size_request(entry, PlaylistColumnLayout::PixelWidth(column, gtk_widget_get_width(widget_)), -1);
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
        if (playlist_ && PlaylistTagCompletion::CompletesColumn(column)) {
          GtkEntryCompletion *completion = gtk_entry_completion_new();
          GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
          for (const std::string &value : PlaylistTagCompletion::UniqueValues(playlist_->songs(), column)) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, value.c_str(), -1);
          }
          gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
          gtk_entry_completion_set_text_column(completion, 0);
          gtk_entry_set_completion(GTK_ENTRY(entry), completion);
          g_object_unref(store);
          g_object_unref(completion);
        }
        GtkEventController *tabs = gtk_event_controller_key_new();
        gtk_event_controller_set_propagation_phase(tabs, GTK_PHASE_CAPTURE);
        gtk_widget_add_controller(entry, tabs);
        g_signal_connect(tabs, "key-pressed",
                         G_CALLBACK((+[](GtkEventControllerKey *controller, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                           auto *self = static_cast<PlaylistView *>(data);
                           const PlaylistEditOrder::TabAction tab =
                               PlaylistEditOrder::FromKey(keyval, (state & GDK_SHIFT_MASK) != 0);
                           if (tab == PlaylistEditOrder::TabAction::None) {
                             return FALSE;
                           }
                           GtkWidget *edited = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
                           const int edited_row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(edited), "row-index"));
                           const PlaylistColumn edited_column =
                               static_cast<PlaylistColumn>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(edited), "column")) - 1);
                           if (self->edit_commit_) {
                             self->edit_commit_(edited_row, edited_column, gtk_editable_get_text(GTK_EDITABLE(edited)));
                           }
                           std::vector<int> rows = self->visible_rows_;
                           if (rows.empty() && self->playlist_) {
                             for (int i = 0; i < self->playlist_->row_count(); ++i) {
                               rows.push_back(i);
                             }
                           }
                           const std::vector<PlaylistColumn> editable = PlaylistEditOrder::EditableVisible(PlaylistColumnLayout::Visible());
                           const PlaylistEditOrder::Cell cell =
                               tab == PlaylistEditOrder::TabAction::Next
                                   ? PlaylistEditOrder::Next(edited_row, edited_column, rows, editable)
                                   : PlaylistEditOrder::Previous(edited_row, edited_column, rows, editable);
                           if (cell.valid) {
                             self->StartInlineEdit(cell.row, cell.column);
                           }
                           return TRUE;
                         })),
                         this);
        gtk_widget_grab_focus(entry);
        return;
      }
    }
    child = gtk_widget_get_next_sibling(child);
  }
}

void PlaylistView::ReloadLookCss() {
  Settings look;
  look.BeginGroup(PlaylistSettings::kSettingsGroup);
  StyleUtils::LoadCss(PlaylistLook::CombinedCss(look.BoolValue(PlaylistSettings::kAlternatingRowColors, PlaylistSettings::kDefaultAlternatingRowColors),
                                                look.BoolValue(PlaylistSettings::kGlowEffect, PlaylistSettings::kDefaultGlowEffect),
                                                look.BoolValue(PlaylistSettings::kShowBars, PlaylistSettings::kDefaultShowBars), playback_progress_,
                                                glow_step_));
}

void PlaylistView::StopGlowTimer() {
  if (glow_timeout_) {
    g_source_remove(glow_timeout_);
    glow_timeout_ = 0;
  }
}

void PlaylistView::StartGlowTimer() {
  if (glow_timeout_) {
    return;
  }
  glow_timeout_ = g_timeout_add(PlaylistLook::GlowIntervalMs(), +[](gpointer data) -> gboolean {
    return static_cast<PlaylistView *>(data)->OnGlowTick();
  }, this);
}

gboolean PlaylistView::OnGlowTick() {
  glow_step_ = PlaylistLook::NextGlowStep(glow_step_);
  ReloadLookCss();
  return G_SOURCE_CONTINUE;
}

void PlaylistView::SetGlowing(bool glowing) {
  glowing_ = glowing;
  Settings look;
  look.BeginGroup(PlaylistSettings::kSettingsGroup);
  const bool glow = look.BoolValue(PlaylistSettings::kGlowEffect, PlaylistSettings::kDefaultGlowEffect);
  const bool bars = look.BoolValue(PlaylistSettings::kShowBars, PlaylistSettings::kDefaultShowBars);
  if (!PlaylistLook::ShouldAnimateGlow(glow, bars, glowing_)) {
    StopGlowTimer();
    glow_step_ = PlaylistLook::StopGlowStep();
    ReloadLookCss();
    return;
  }
  StartGlowTimer();
}

void PlaylistView::SetPlaybackProgress(double progress) {
  playback_progress_ = std::clamp(progress, 0.0, 1.0);
  ReloadLookCss();
}

void PlaylistView::SetPaused(bool paused) { paused_ = paused; }

void PlaylistView::OnShown() {
  if (PlaylistAutoscroll::ShouldRestartGlowOnShow(glowing_)) {
    StartGlowTimer();
  }
  if (playlist_ && PlaylistAutoscroll::ShouldRunOnShow()) {
    MaybeScrollToRow(playlist_->current_row(), PlaylistAutoscroll::ShowMode());
  }
}

void PlaylistView::OnHidden() {
  if (PlaylistAutoscroll::ShouldStopGlowOnHide()) {
    StopGlowTimer();
  }
}

void PlaylistView::StopBackgroundFade() {
  if (background_fade_id_) {
    g_source_remove(background_fade_id_);
    background_fade_id_ = 0;
  }
  background_fade_elapsed_ms_ = 0;
}

void PlaylistView::ApplyBackgroundCss() {
  std::string css = background_css_;
  if (!previous_background_css_.empty()) {
    css += AppearanceBackgroundFade::RewriteSelector(previous_background_css_, Appearance::kPlaylistViewportSelector,
                                                     AppearanceBackgroundFade::kPreviousSelector());
  }
  if (!css.empty()) {
    StyleUtils::LoadCss(css);
  }
}

void PlaylistView::SetBackground(const std::string &css, const std::string &key) {
  if (!AppearanceBackgroundFade::ShouldReplace(background_key_, key)) {
    return;
  }
  const bool animate = AppearanceBackgroundFade::ShouldAnimate(widget_ && gtk_widget_get_mapped(widget_), !background_key_.empty());
  previous_background_css_ = background_css_;
  background_css_ = css;
  background_key_ = key;
  StopBackgroundFade();
  ApplyBackgroundCss();
  if (!animate || !previous_bg_ || !current_bg_) {
    if (previous_bg_) {
      gtk_widget_set_opacity(previous_bg_, 0.0);
    }
    if (current_bg_) {
      gtk_widget_set_opacity(current_bg_, 1.0);
    }
    return;
  }
  gtk_widget_set_opacity(previous_bg_, AppearanceBackgroundFade::PreviousOpacity(0));
  gtk_widget_set_opacity(current_bg_, AppearanceBackgroundFade::CurrentOpacity(0));
  background_fade_id_ = g_timeout_add(static_cast<guint>(AppearanceBackgroundFade::kTickMs),
                                      +[](gpointer data) -> gboolean { return static_cast<PlaylistView *>(data)->OnBackgroundFadeTick(); },
                                      this);
}

gboolean PlaylistView::OnBackgroundFadeTick() {
  background_fade_elapsed_ms_ += AppearanceBackgroundFade::kTickMs;
  const double previous = AppearanceBackgroundFade::PreviousOpacity(background_fade_elapsed_ms_);
  if (previous_bg_) {
    gtk_widget_set_opacity(previous_bg_, previous);
  }
  if (current_bg_) {
    gtk_widget_set_opacity(current_bg_, AppearanceBackgroundFade::CurrentOpacity(background_fade_elapsed_ms_));
  }
  if (AppearanceBackgroundFade::ShouldClearPrevious(previous)) {
    previous_background_css_.clear();
    ApplyBackgroundCss();
    background_fade_id_ = 0;
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}

void PlaylistView::InhibitAutoscroll() {
  inhibit_autoscroll_ = true;
  if (inhibit_timeout_) {
    g_source_remove(inhibit_timeout_);
  }
  inhibit_timeout_ = g_timeout_add(PlaylistAutoscroll::kGraceMs, +[](gpointer data) -> gboolean {
    auto *self = static_cast<PlaylistView *>(data);
    self->inhibit_autoscroll_ = false;
    self->inhibit_timeout_ = 0;
    return G_SOURCE_REMOVE;
  }, this);
}

bool PlaylistView::IsRowVisible(int row) const {
  GtkWidget *child = gtk_widget_get_first_child(grid_);
  while (child) {
    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-index")) == row) {
      graphene_rect_t bounds{};
      if (!gtk_widget_compute_bounds(child, widget_, &bounds)) {
        return false;
      }
      const int view_h = gtk_widget_get_height(widget_);
      return bounds.origin.y >= 0 && bounds.origin.y + bounds.size.height <= static_cast<float>(view_h);
    }
    child = gtk_widget_get_next_sibling(child);
  }
  return false;
}

void PlaylistView::MaybeScrollToRow(int row, Playlist::AutoScroll mode) {
  if (!PlaylistAutoscroll::ShouldScroll(mode, inhibit_autoscroll_)) {
    return;
  }
  if (mode != Playlist::AutoScroll::Always && PlaylistAutoscroll::ShouldSkipIfVisible(IsRowVisible(row))) {
    return;
  }
  if (mode == Playlist::AutoScroll::Always) {
    inhibit_autoscroll_ = false;
  }
  ScrollToRow(row, true);
}

void PlaylistView::CopyCurrentToClipboard() {
  if (!playlist_ || selected_rows_.empty()) {
    return;
  }
  const int row = selected_rows_.front();
  if (row < 0 || row >= playlist_->row_count()) {
    return;
  }
  const Song &song = playlist_->songs()[static_cast<size_t>(row)];
  std::vector<std::string> texts;
  for (PlaylistColumn column : PlaylistColumnLayout::Visible()) {
    texts.push_back(PlaylistDelegates::ColumnText(song, column));
  }
  const PlaylistClipboard::CopyPayload payload = PlaylistClipboard::FromSong(song, texts);
  GdkClipboard *clipboard = gtk_widget_get_clipboard(widget_);
  if (payload.urls.empty()) {
    gdk_clipboard_set_text(clipboard, payload.display_text.c_str());
    return;
  }
  const std::string uri_list = PlaylistClipboard::UriList(payload.urls);
  GBytes *bytes = g_bytes_new(uri_list.data(), uri_list.size());
  GdkContentProvider *uris = gdk_content_provider_new_for_bytes("text/uri-list", bytes);
  g_bytes_unref(bytes);
  GdkContentProvider *text = gdk_content_provider_new_typed(G_TYPE_STRING, payload.display_text.c_str());
  GdkContentProvider *parts[] = {text, uris};
  GdkContentProvider *all = gdk_content_provider_new_union(parts, 2);
  gdk_clipboard_set_content(clipboard, all);
  g_object_unref(all);
  g_object_unref(text);
  g_object_unref(uris);
}

void PlaylistView::JumpToCurrentlyPlayingTrack() {
  if (!playlist_ || !PlaylistFilterDelay::ShouldJumpToPlaying(playlist_->current_row())) {
    return;
  }
  ScrollToRow(playlist_->current_row(), true);
}

void PlaylistView::JumpToLastPlayedTrack() {
  if (!playlist_ || !PlaylistRestoreScroll::ShouldJump(playlist_->last_played_row())) {
    return;
  }
  selected_rows_ = {playlist_->last_played_row()};
  ScrollToRow(playlist_->last_played_row(), true);
}

void PlaylistView::ScrollToRow(int row, bool center) {
  GtkWidget *child = gtk_widget_get_first_child(grid_);
  while (child) {
    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-index")) == row) {
      graphene_rect_t bounds;
      if (gtk_widget_compute_bounds(child, widget_, &bounds)) {
        GtkAdjustment *adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(widget_));
        double value = bounds.origin.y;
        if (center) {
          value = PlaylistAutoscroll::CenteredOffset(static_cast<int>(bounds.origin.y), static_cast<int>(bounds.size.height),
                                                     gtk_widget_get_height(widget_));
        }
        gtk_adjustment_set_value(adjust, value);
      }
      return;
    }
    child = gtk_widget_get_next_sibling(child);
  }
}
