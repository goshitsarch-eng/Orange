#include "queue/queueview.h"

#include "core/appearanceleftpanel.h"
#include "playlist/playlistdropindicator.h"
#include "queue/queue.h"
#include "queue/queuedrop.h"
#include "queue/queuekeyboard.h"
#include "queue/queueui.h"
#include "utilities/styleutils.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"

#include <algorithm>

QueueView::QueueView(Queue *queue) : queue_(queue) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(widget_, 8);
  gtk_widget_set_margin_end(widget_, 8);
  gtk_widget_set_margin_top(widget_, 8);
  gtk_widget_set_margin_bottom(widget_, 8);
  list_ = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_widget_set_vexpand(list_, TRUE);
  StyleUtils::LoadCss(QueueUi::AlternatingCss(), StyleUtils::Slot::kQueueLook);
  GtkWidget *overlay = gtk_overlay_new();
  gtk_overlay_set_child(GTK_OVERLAY(overlay), list_);
  drop_overlay_ = gtk_drawing_area_new();
  gtk_widget_set_can_target(drop_overlay_, FALSE);
  gtk_widget_set_hexpand(drop_overlay_, TRUE);
  gtk_widget_set_vexpand(drop_overlay_, TRUE);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drop_overlay_),
                                 +[](GtkDrawingArea *, cairo_t *cr, int width, int, gpointer data) {
                                   auto *self = static_cast<QueueView *>(data);
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
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), drop_overlay_);
  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), overlay);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_box_append(GTK_BOX(widget_), scrolled);
  GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  move_down_ = gtk_button_new_from_icon_name(QueueUi::MoveDownIcon());
  move_up_ = gtk_button_new_from_icon_name(QueueUi::MoveUpIcon());
  remove_ = gtk_button_new_from_icon_name(QueueUi::RemoveIcon());
  clear_ = gtk_button_new_from_icon_name(QueueUi::ClearIcon());
  gtk_widget_add_css_class(move_down_, "flat");
  gtk_widget_add_css_class(move_up_, "flat");
  gtk_widget_add_css_class(remove_, "flat");
  gtk_widget_add_css_class(clear_, "flat");
  gtk_widget_set_tooltip_text(move_down_, QueueUi::MoveDownTooltip());
  gtk_widget_set_tooltip_text(move_up_, QueueUi::MoveUpTooltip());
  gtk_widget_set_tooltip_text(remove_, QueueUi::RemoveTooltip());
  gtk_widget_set_tooltip_text(clear_, QueueUi::ClearTooltip());
  summary_ = gtk_label_new("");
  gtk_widget_add_css_class(summary_, "dim-label");
  gtk_widget_set_hexpand(summary_, TRUE);
  gtk_widget_set_halign(summary_, GTK_ALIGN_END);
  g_signal_connect(move_up_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<QueueView *>(data)->MoveUp(); }), this);
  g_signal_connect(move_down_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<QueueView *>(data)->MoveDown(); }), this);
  g_signal_connect(remove_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<QueueView *>(data)->Remove(); }), this);
  g_signal_connect(clear_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<QueueView *>(data)->Clear(); }), this);
  gtk_box_append(GTK_BOX(buttons), move_down_);
  gtk_box_append(GTK_BOX(buttons), move_up_);
  gtk_box_append(GTK_BOX(buttons), remove_);
  gtk_box_append(GTK_BOX(buttons), clear_);
  gtk_box_append(GTK_BOX(buttons), summary_);
  gtk_box_prepend(GTK_BOX(widget_), buttons);
  g_signal_connect(list_, "selected-rows-changed", G_CALLBACK(+[](GtkListBox *, gpointer data) {
                     static_cast<QueueView *>(data)->UpdateChrome();
                   }),
                   this);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<QueueView *>(data);
                     const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-index")) - 1;
                     if (!self->queue_ || !self->activate_ || index < 0) {
                       return;
                     }
                     const SongList songs = self->queue_->songs();
                     if (index < static_cast<int>(songs.size())) {
                       self->activate_(songs[static_cast<size_t>(index)]);
                     }
                   }),
                   this);
  GtkDropTarget *target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
#ifdef GDK_TYPE_FILE_LIST
  GType types[] = {G_TYPE_STRING, GDK_TYPE_FILE_LIST};
  gtk_drop_target_set_gtypes(target, types, 2);
#endif
  gtk_drop_target_set_actions(target, static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE));
  gtk_drop_target_set_preload(target, TRUE);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(target));
  g_signal_connect(target, "motion", G_CALLBACK(+[](GtkDropTarget *, gdouble, gdouble y, gpointer data) -> GdkDragAction {
                     static_cast<QueueView *>(data)->UpdateDropIndicator(y);
                     return GDK_ACTION_COPY;
                   }),
                   this);
  g_signal_connect(target, "leave", G_CALLBACK(+[](GtkDropTarget *, gpointer data) {
                     static_cast<QueueView *>(data)->ClearDropIndicator();
                   }),
                   this);
  g_signal_connect(target, "drop", G_CALLBACK((+[](GtkDropTarget *, const GValue *value, gdouble, gdouble y, gpointer data) -> gboolean {
                     return static_cast<QueueView *>(data)->OnDrop(value, y);
                   })),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     return static_cast<QueueView *>(data)->OnKeyPressed(keyval, state);
                   })),
                   this);
  SetQueue(queue);
  ApplyLook();
}

void QueueView::ApplyLook() {
  if (!AppearanceLeftPanel::ShouldApply()) {
    return;
  }
  const int size = AppearanceLeftPanel::StoredSize();
  AppearanceLeftPanel::ApplyWidget(move_down_, size);
  AppearanceLeftPanel::ApplyWidget(move_up_, size);
  AppearanceLeftPanel::ApplyWidget(remove_, size);
  AppearanceLeftPanel::ApplyWidget(clear_, size);
}

void QueueView::SetQueue(Queue *queue) {
  queue_ = queue;
  if (queue_) {
    queue_->Changed.Connect([this]() { Reload(); });
  }
  Reload();
}

QueueView::~QueueView() { ResetTypeAhead(); }

void QueueView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

gboolean QueueView::OnKeyPressed(guint keyval, GdkModifierType modifiers) {
  switch (QueueKeyboard::FromKey(keyval, static_cast<unsigned>(modifiers), GDK_CONTROL_MASK)) {
    case QueueKeyboard::Action::Remove:
      Remove();
      return TRUE;
    case QueueKeyboard::Action::Clear:
      Clear();
      return TRUE;
    case QueueKeyboard::Action::MoveUp:
      MoveUp();
      return TRUE;
    case QueueKeyboard::Action::MoveDown:
      MoveDown();
      return TRUE;
    case QueueKeyboard::Action::None:
      break;
  }
  const ListBoxKeyboard::Action action = ListBoxKeyboard::FromKey(keyval);
  if (action == ListBoxKeyboard::Action::Activate) {
    ListBoxKeyboardGtk::ActivateSelected(list_);
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::Delete) {
    Remove();
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::MoveUp || action == ListBoxKeyboard::Action::MoveDown || action == ListBoxKeyboard::Action::Home ||
      action == ListBoxKeyboard::Action::End) {
    ListBoxKeyboardGtk::SelectIndex(list_, ListBoxKeyboard::NextIndex(ListBoxKeyboardGtk::SelectedIndex(list_),
                                                                      ListBoxKeyboardGtk::Count(list_), action));
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::Escape) {
    ResetTypeAhead();
    return TRUE;
  }
  const gunichar ch = gdk_keyval_to_unicode(keyval);
  if (ch && g_unichar_isprint(ch)) {
    gchar utf8[8] = {};
    typeahead_.append(utf8, static_cast<size_t>(g_unichar_to_utf8(ch, utf8)));
    if (typeahead_timeout_) {
      g_source_remove(typeahead_timeout_);
    }
    typeahead_timeout_ = g_timeout_add(1000, [](gpointer data) -> gboolean {
      auto *self = static_cast<QueueView *>(data);
      self->typeahead_timeout_ = 0;
      self->typeahead_.clear();
      return G_SOURCE_REMOVE;
    }, this);
    const int index = ListBoxKeyboard::FirstPrefixIndex(ListBoxKeyboardGtk::Labels(list_), typeahead_);
    if (index >= 0) {
      ListBoxKeyboardGtk::SelectIndex(list_, index);
    }
    return TRUE;
  }
  return FALSE;
}

void QueueView::SetActivateCallback(std::function<void(const Song &)> callback) { activate_ = std::move(callback); }

void QueueView::SetNowPlayingUrl(const std::string &url) {
  now_playing_url_ = url;
  Rebuild();
}

void QueueView::Reload() { Rebuild(); }

void QueueView::Rebuild() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  if (!queue_ || queue_->empty()) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new("Queue is empty");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
    UpdateChrome();
    return;
  }
  int index = 0;
  for (const Song &song : queue_->songs()) {
    GtkWidget *row = gtk_list_box_row_new();
    const bool current = IsNowPlaying(song, now_playing_url_);
    const std::string text = (current ? "▶ " : "") + song.PrettyTitleWithArtist();
    GtkWidget *label = gtk_label_new(text.c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    if (current) {
      gtk_widget_add_css_class(row, "accent");
    }
    gtk_widget_add_css_class(row, QueueUi::kRowClass);
    if (QueueUi::IsAltRow(index)) {
      gtk_widget_add_css_class(row, QueueUi::kAltClass);
    }
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data(G_OBJECT(row), "row-index", GINT_TO_POINTER(index + 1));
    SetupRowDrag(row, index);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
    ++index;
  }
  UpdateChrome();
}

void QueueView::UpdateChrome() {
  const int count = queue_ ? queue_->size() : 0;
  if (summary_) {
    gtk_label_set_text(GTK_LABEL(summary_), QueueUi::SummaryText(queue_ ? queue_->songs() : SongList{}).c_str());
  }
  const QueueUi::ButtonState state = QueueUi::Buttons(SelectedIndexes(), count);
  if (move_up_) {
    gtk_widget_set_sensitive(move_up_, state.move_up);
  }
  if (move_down_) {
    gtk_widget_set_sensitive(move_down_, state.move_down);
  }
  if (remove_) {
    gtk_widget_set_sensitive(remove_, state.remove);
  }
  if (clear_) {
    gtk_widget_set_sensitive(clear_, state.clear);
  }
}

void QueueView::SetupRowDrag(GtkWidget *row, int index) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_MOVE);
  g_object_set_data(G_OBJECT(src), "row", GINT_TO_POINTER(index + 1));
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<QueueView *>(data);
                     const int r = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(s), "row")) - 1;
                     std::vector<int> rows = self->SelectedIndexes();
                     if (std::find(rows.begin(), rows.end(), r) == rows.end()) {
                       rows = {r};
                     }
                     if (rows.empty() || r < 0) {
                       return nullptr;
                     }
                     const std::string payload = QueueDrop::RowsPayload(rows, QueueDrop::kQueueRowsPrefix);
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

int QueueView::RowAtY(double y) const {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  int last_index = 0;
  while (child) {
    graphene_rect_t bounds{};
    const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-index"));
    if (stored > 0 && gtk_widget_compute_bounds(child, list_, &bounds)) {
      const int index = stored - 1;
      if (y < bounds.origin.y + bounds.size.height) {
        return index;
      }
      last_index = index + 1;
    }
    child = gtk_widget_get_next_sibling(child);
  }
  return last_index;
}

void QueueView::ClearDropIndicator() {
  drop_state_ = {};
  if (drop_overlay_) {
    gtk_widget_queue_draw(drop_overlay_);
  }
}

void QueueView::UpdateDropIndicator(double y) {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  double last_bottom = 0;
  int found = -1;
  double row_y = 0;
  double row_h = 0;
  bool has_rows = false;
  while (child) {
    graphene_rect_t bounds{};
    const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-index"));
    if (stored > 0 && gtk_widget_compute_bounds(child, list_, &bounds)) {
      has_rows = true;
      last_bottom = bounds.origin.y + bounds.size.height;
      if (y < last_bottom && found < 0) {
        found = stored - 1;
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

gboolean QueueView::OnDrop(const GValue *value, double y) {
  UpdateDropIndicator(y);
  const int dest = PlaylistDropIndicator::InsertRow(drop_state_, RowAtY(y));
  ClearDropIndicator();
  if (G_VALUE_HOLDS_STRING(value)) {
    const char *text = g_value_get_string(value);
    const std::string payload = text ? text : "";
    if (QueueDrop::IsQueueRows(payload)) {
      const std::vector<int> rows = QueueDrop::ParseRows(payload, QueueDrop::kQueueRowsPrefix);
      if (queue_ && !rows.empty()) {
        queue_->MoveRows(rows, dest);
        return TRUE;
      }
    }
    if (QueueDrop::IsPlaylistRows(payload)) {
      const std::vector<int> rows = QueueDrop::ParseRows(payload, QueueDrop::kPlaylistRowsPrefix);
      if (playlist_drop_ && !rows.empty()) {
        playlist_drop_(rows, dest);
        return TRUE;
      }
    }
    const std::vector<std::string> urls = QueueDrop::ParseUrls(payload);
    if (url_drop_ && !urls.empty()) {
      url_drop_(urls, dest);
      return TRUE;
    }
  }
#ifdef GDK_TYPE_FILE_LIST
  if (G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST) && url_drop_) {
    std::vector<std::string> urls;
    auto *list = static_cast<GdkFileList *>(g_value_get_boxed(value));
    GSList *files = gdk_file_list_get_files(list);
    for (GSList *item = files; item; item = item->next) {
      gchar *uri = g_file_get_uri(G_FILE(item->data));
      if (uri) {
        urls.emplace_back(uri);
        g_free(uri);
      }
    }
    if (!urls.empty()) {
      url_drop_(urls, dest);
      return TRUE;
    }
  }
#endif
  return FALSE;
}

std::vector<int> QueueView::SelectedIndexes() const {
  std::vector<int> indexes;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-index")) - 1;
        if (index >= 0) {
          static_cast<std::vector<int> *>(data)->push_back(index);
        }
      },
      &indexes);
  std::sort(indexes.begin(), indexes.end());
  return indexes;
}

void QueueView::MoveUp() {
  const std::vector<int> indexes = SelectedIndexes();
  if (!queue_ || indexes.empty() || indexes.front() <= 0) {
    return;
  }
  queue_->MoveRows(indexes, indexes.front() - 1);
}

void QueueView::MoveDown() {
  const std::vector<int> indexes = SelectedIndexes();
  if (!queue_ || indexes.empty() || indexes.back() + 1 >= queue_->size()) {
    return;
  }
  queue_->MoveRows(indexes, indexes.back() + 2);
}

void QueueView::Remove() {
  const std::vector<int> indexes = SelectedIndexes();
  if (queue_ && !indexes.empty()) {
    queue_->RemoveRows(indexes);
  }
}

void QueueView::Clear() {
  if (queue_) {
    queue_->Clear();
  }
}
