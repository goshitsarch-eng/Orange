#include "smartplaylists/smartplaylistsearchpreview.h"

#include "smartplaylists/smartplaylistpreviewpolicy.h"

SmartPlaylistSearchPreview::SmartPlaylistSearchPreview() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  label_ = gtk_label_new("Preview");
  gtk_widget_set_halign(label_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(widget_), label_);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_widget_set_size_request(scroll, -1, 140);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list_);
  gtk_box_append(GTK_BOX(widget_), scroll);
}

void SmartPlaylistSearchPreview::Update(const SmartPlaylistSearch &search, const SongList &songs) {
  if (have_last_search_ && SmartPlaylistPreviewPolicy::SameSearch(search, last_search_)) {
    return;
  }
  have_last_search_ = true;
  last_search_ = search;
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  const SongList matches = search.Search(songs);
  match_count_ = static_cast<int>(matches.size());
  gtk_label_set_text(GTK_LABEL(label_), (std::to_string(match_count_) + " matching songs").c_str());
  int shown = 0;
  for (const Song &song : matches) {
    if (shown++ >= 8) {
      break;
    }
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *title = gtk_label_new(song.PrettyTitleWithArtist().c_str());
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_margin_start(title, 8);
    gtk_widget_set_margin_end(title, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), title);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}
