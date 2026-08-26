#include "streaming/streamingsearchview.h"

#include "streaming/streamingsearchitemdelegate.h"
#include "translations/translations.h"

StreamingSearchView::StreamingSearchView(StreamingService *service) : service_(service) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  search_entry_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search_entry_), Translations::CStr("Search"));
  gtk_widget_set_margin_start(search_entry_, 8);
  gtk_widget_set_margin_end(search_entry_, 8);
  gtk_widget_set_margin_top(search_entry_, 6);
  gtk_widget_set_margin_bottom(search_entry_, 4);
  GtkWidget *types = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(types, 8);
  gtk_widget_set_margin_end(types, 8);
  gtk_widget_set_margin_bottom(types, 4);
  type_artists_ = gtk_toggle_button_new_with_label("Artists");
  type_albums_ = gtk_toggle_button_new_with_label("Albums");
  type_songs_ = gtk_toggle_button_new_with_label("Songs");
  gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(type_albums_), GTK_TOGGLE_BUTTON(type_artists_));
  gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(type_songs_), GTK_TOGGLE_BUTTON(type_artists_));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(type_songs_), TRUE);
  gtk_box_append(GTK_BOX(types), type_artists_);
  gtk_box_append(GTK_BOX(types), type_albums_);
  gtk_box_append(GTK_BOX(types), type_songs_);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  list_ = gtk_list_box_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list_);
  auto search_now = +[](GtkWidget *, gpointer data) {
    auto *self = static_cast<StreamingSearchView *>(data);
    const char *text = gtk_editable_get_text(GTK_EDITABLE(self->search_entry_));
    self->Search(text ? text : "");
  };
  g_signal_connect(search_entry_, "activate", G_CALLBACK(search_now), this);
  g_signal_connect(search_entry_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     const std::string query = text ? text : "";
                     if (query.size() >= 2) {
                       self->Search(query);
                     }
                   }),
                   this);
  g_signal_connect(type_artists_, "toggled", G_CALLBACK(search_now), this);
  g_signal_connect(type_albums_, "toggled", G_CALLBACK(search_now), this);
  g_signal_connect(type_songs_, "toggled", G_CALLBACK(search_now), this);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (song && self->activate_) {
                       self->activate_(*song);
                     }
                   }),
                   this);
  gtk_box_append(GTK_BOX(widget_), search_entry_);
  gtk_box_append(GTK_BOX(widget_), types);
  gtk_box_append(GTK_BOX(widget_), scroll);
}

StreamingSearchView::~StreamingSearchView() = default;

void StreamingSearchView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void StreamingSearchView::Search(const std::string &query) {
  if (!service_ || query.empty()) {
    return;
  }
  StreamingService::SearchType type = StreamingService::SearchType::Songs;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(type_artists_))) {
    type = StreamingService::SearchType::Artists;
  } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(type_albums_))) {
    type = StreamingService::SearchType::Albums;
  }
  model_.SetSearchType(type);
  service_->Search(query, type, [this](const SongList &songs) {
    model_.SetSongs(songs);
    Rebuild();
  });
}

void StreamingSearchView::Rebuild() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  const SongList visible = sort_model_.Visible();
  if (visible.empty()) {
    gtk_list_box_append(GTK_LIST_BOX(list_), gtk_label_new(Translations::CStr("No results")));
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
    g_object_set_data_full(G_OBJECT(row), "row-data", new Song(song), [](gpointer p) { delete static_cast<Song *>(p); });
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}
