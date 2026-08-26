#include "radios/radiobrowsersearchview.h"

RadioBrowserSearchView::RadioBrowserSearchView() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  entry_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(entry_), "Search Radio Browser");
  gtk_widget_set_margin_start(entry_, 8);
  gtk_widget_set_margin_end(entry_, 8);
  gtk_widget_set_margin_top(entry_, 6);
  gtk_widget_set_margin_bottom(entry_, 4);
  gtk_box_append(GTK_BOX(widget_), entry_);
  g_signal_connect(entry_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<RadioBrowserSearchView *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     if (self->changed_) {
                       self->changed_(text ? text : "");
                     }
                   }),
                   this);
}

void RadioBrowserSearchView::SetResults(const std::vector<RadioChannel> &results) { model_.SetResults(results); }
