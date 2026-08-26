#include "streaming/streamingcollectionview.h"

#include "streaming/streamingsearchitemdelegate.h"
#include "utilities/strutils.h"

StreamingCollectionView::StreamingCollectionView(const std::string &title) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(header, 8);
  gtk_widget_set_margin_end(header, 8);
  gtk_widget_set_margin_top(header, 6);
  gtk_widget_set_margin_bottom(header, 4);
  GtkWidget *label = gtk_label_new(title.c_str());
  gtk_widget_set_hexpand(label, TRUE);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  GtkWidget *refresh = gtk_button_new_from_icon_name("view-refresh-symbolic");
  gtk_widget_set_tooltip_text(refresh, "Refresh");
  g_signal_connect(refresh, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     if (self->refresh_) {
                       self->refresh_();
                     }
                   }),
                   this);
  gtk_box_append(GTK_BOX(header), label);
  gtk_box_append(GTK_BOX(header), refresh);
  filter_entry_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(filter_entry_), "Filter");
  gtk_widget_set_margin_start(filter_entry_, 8);
  gtk_widget_set_margin_end(filter_entry_, 8);
  g_signal_connect(filter_entry_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     self->SetFilter(text ? text : "");
                   }),
                   this);
  status_label_ = gtk_label_new("");
  gtk_widget_set_margin_start(status_label_, 8);
  gtk_widget_set_halign(status_label_, GTK_ALIGN_START);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  list_ = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list_);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (song && self->activate_) {
                       self->activate_(*song);
                     }
                   }),
                   this);
  gtk_box_append(GTK_BOX(widget_), header);
  gtk_box_append(GTK_BOX(widget_), filter_entry_);
  gtk_box_append(GTK_BOX(widget_), status_label_);
  gtk_box_append(GTK_BOX(widget_), scroll);
}

StreamingCollectionView::~StreamingCollectionView() = default;

void StreamingCollectionView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void StreamingCollectionView::SetRefreshCallback(RefreshCallback callback) { refresh_ = std::move(callback); }

void StreamingCollectionView::SetFilter(const std::string &filter) {
  filter_ = filter;
  Rebuild();
}

void StreamingCollectionView::SetStatus(const std::string &status) { gtk_label_set_text(GTK_LABEL(status_label_), status.c_str()); }

void StreamingCollectionView::SetSongs(const SongList &songs) {
  songs_ = songs;
  Rebuild();
}

SongList StreamingCollectionView::Visible() const {
  if (filter_.empty()) {
    return songs_;
  }
  SongList visible;
  for (const Song &song : songs_) {
    if (StrUtils::ContainsInsensitive(song.PrettyTitleWithArtist(), filter_) || StrUtils::ContainsInsensitive(song.album(), filter_)) {
      visible.push_back(song);
    }
  }
  return visible;
}

void StreamingCollectionView::Rebuild() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  const SongList visible = Visible();
  if (visible.empty()) {
    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), gtk_label_new(songs_.empty() ? "No items. Refresh to load." : "No matches"));
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
    SetStatus(songs_.empty() ? "0 items" : "0 shown");
    return;
  }
  for (const Song &song : visible) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 4);
    gtk_widget_set_margin_bottom(box, 4);
    GtkWidget *primary = gtk_label_new(StreamingSearchItemDelegate::PrimaryText(song).c_str());
    gtk_widget_set_halign(primary, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), primary);
    const std::string secondary = StreamingSearchItemDelegate::SecondaryText(song);
    if (!secondary.empty()) {
      GtkWidget *sub = gtk_label_new(secondary.c_str());
      gtk_widget_add_css_class(sub, "dim-label");
      gtk_widget_set_halign(sub, GTK_ALIGN_START);
      gtk_box_append(GTK_BOX(box), sub);
    }
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    auto *copy = new Song(song);
    g_object_set_data_full(G_OBJECT(row), "row-data", copy, [](gpointer p) { delete static_cast<Song *>(p); });
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
  SetStatus(std::to_string(visible.size()) + " items");
}
