#include "queue/queueview.h"

#include "queue/queue.h"

QueueView::QueueView(Queue *queue) : queue_(queue) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(widget_, 8);
  gtk_widget_set_margin_end(widget_, 8);
  gtk_widget_set_margin_top(widget_, 8);
  gtk_widget_set_margin_bottom(widget_, 8);
  list_ = gtk_list_box_new();
  gtk_widget_set_vexpand(list_, TRUE);
  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_box_append(GTK_BOX(widget_), scrolled);
  GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *up = gtk_button_new_with_label("Up");
  GtkWidget *down = gtk_button_new_with_label("Down");
  GtkWidget *remove = gtk_button_new_with_label("Remove");
  GtkWidget *clear = gtk_button_new_with_label("Clear");
  g_signal_connect(up, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<QueueView *>(data)->MoveUp(); }), this);
  g_signal_connect(down, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<QueueView *>(data)->MoveDown(); }), this);
  g_signal_connect(remove, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<QueueView *>(data)->Remove(); }), this);
  g_signal_connect(clear, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<QueueView *>(data)->Clear(); }), this);
  gtk_box_append(GTK_BOX(buttons), up);
  gtk_box_append(GTK_BOX(buttons), down);
  gtk_box_append(GTK_BOX(buttons), remove);
  gtk_box_append(GTK_BOX(buttons), clear);
  gtk_box_append(GTK_BOX(widget_), buttons);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<QueueView *>(data);
                     const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-index"));
                     if (!self->queue_ || !self->activate_) {
                       return;
                     }
                     const SongList songs = self->queue_->songs();
                     if (index >= 0 && index < static_cast<int>(songs.size())) {
                       self->activate_(songs[static_cast<size_t>(index)]);
                     }
                   }),
                   this);
  if (queue_) {
    queue_->Changed.Connect([this]() { Reload(); });
  }
  Reload();
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
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data(G_OBJECT(row), "row-index", GINT_TO_POINTER(index++));
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

int QueueView::SelectedIndex() const {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  if (!row) {
    return -1;
  }
  return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-index"));
}

void QueueView::MoveUp() {
  const int index = SelectedIndex();
  if (queue_ && index > 0) {
    queue_->Move(index, index - 1);
  }
}

void QueueView::MoveDown() {
  const int index = SelectedIndex();
  if (queue_ && index >= 0 && index + 1 < queue_->size()) {
    queue_->Move(index, index + 1);
  }
}

void QueueView::Remove() {
  const int index = SelectedIndex();
  if (queue_ && index >= 0) {
    queue_->Remove(index);
  }
}

void QueueView::Clear() {
  if (queue_) {
    queue_->Clear();
  }
}
