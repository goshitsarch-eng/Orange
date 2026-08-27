#include "searchfield.h"

SearchField::SearchField() {
  entry_ = gtk_search_entry_new();
  g_object_ref_sink(entry_);
  gtk_search_entry_set_search_delay(GTK_SEARCH_ENTRY(entry_), 150);

  g_signal_connect(entry_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer self) {
                     auto *w = static_cast<SearchField *>(self);
                     if (w->changed_cb_) w->changed_cb_(w->text());
                   }),
                   this);
  g_signal_connect(entry_, "activate", G_CALLBACK(+[](GtkSearchEntry *, gpointer self) {
                     auto *w = static_cast<SearchField *>(self);
                     if (w->activated_cb_) w->activated_cb_(w->text());
                   }),
                   this);
}

SearchField::~SearchField() {
  if (entry_) g_object_unref(entry_);
}

std::string SearchField::text() const {
  return gtk_editable_get_text(GTK_EDITABLE(entry_));
}

void SearchField::SetText(const std::string &text) {
  gtk_editable_set_text(GTK_EDITABLE(entry_), text.c_str());
}

void SearchField::Clear() { SetText({}); }
